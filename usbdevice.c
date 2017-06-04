/*
 * Copyright (C) 2017 Dongjin Kim <tobetter@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <termios.h>

#include <libxml/xmlreader.h>

#if defined(__ANDROID__)
#include <libusb.h>
#include <cutils/log.h>
#define printf ALOGE
#define  LOG_TAG  "libmbm-gps"
#else
#include <libusb-1.0/libusb.h>
#endif

#define GPS_DEVICE_LIST_FILE	"/system/etc/odroid-usbgps.xml"

struct gpsdevice {
	uint16_t vid;
	uint16_t pid;
	speed_t baudrate;
};

static struct gpsdevice **devices = NULL;
static int nr_devices = 0;
static int last = 0;

static char *__default_device = NULL;
static speed_t __default_baudrate = B9600;

/*
 *
 */
static void __device_release_pool(struct gpsdevice** devices)
{
	if (devices) {
		struct gpsdevice **devs = devices;

		/* release device entries */
		for (; *devs; devs++) {
			if (*devs)
				free(*devs);
		}

		/* release device pool */
		free(devices);
		devices = NULL;

		nr_devices = 0;
		last = 0;
	}
}

/*
 *
 */
static int __device_create_pool(int nr)
{
	if (0 >= nr)
		return -EINVAL;

	__device_release_pool(devices);

	size_t size = sizeof(struct gpsdevice*) * (nr + 1);
	devices = (struct gpsdevice**)malloc(size);
	if (NULL == devices)
		return -ENOMEM;

	memset(devices, 0, size);

	nr_devices = nr;
	last = 0;

	return 0;
}

#define ARRAY_SIZE(x)		(int)(sizeof(x) / sizeof(x[0]))

static const struct {
	const char* str;
	speed_t baudrate;
} termbits[] = {
	{ "2400", B2400 },
	{ "4800", B4800 },
	{ "9600", B9600 },
};

static speed_t __termbits(const char* str)
{
	if (str) {
		int i;
		for (i = 0; i < ARRAY_SIZE(termbits); i++) {
			if (0 == strcmp(termbits[i].str, str))
				return termbits[i].baudrate;
		}
	}

	return __default_baudrate;
}

static const char* nameof_termbits(speed_t baudrate)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(termbits); i++) {
		if (termbits[i].baudrate == baudrate)
			return termbits[i].str;
	}

	return "unknown";
}

/*
 *
 */
static int __device_add(xmlNode *node)
{
	int ret = 0;

	if (NULL == node)
		return -EINVAL;

	/* USB device attributes */
	char *vid = (char*)xmlGetProp(node, (xmlChar*)"vid");
	char *pid = (char*)xmlGetProp(node, (xmlChar*)"pid");
	char *baudrate = (char*)xmlGetProp(node,
			(xmlChar*)"baudrate");

	/* idVendor and idProduct must be specified */
	if ((NULL == vid) || (NULL == pid)) {
		printf("idVendor or idProduct is missing\n");
		ret = -EINVAL;
		goto exit_device_add;
	}

	struct gpsdevice *dev =
		(struct gpsdevice*)malloc(sizeof(struct gpsdevice));
	if (NULL == dev) {
		ret = -EINVAL;
		goto exit_device_add;
	}

	/* convert string value to interger */
	dev->vid = strtol(vid, NULL, 16);	/* idVendor */
	dev->pid = strtol(pid, NULL, 16);	/* idProduct */
	dev->baudrate = __termbits(baudrate);	/* Baudrate */

	devices[last++] = dev;

exit_device_add:
	if (vid)
		xmlFree(vid);
	if (pid)
		xmlFree(pid);
	if (baudrate)
		xmlFree(baudrate);

	return ret;
}

/*
 *
 */
static int set_default_device(const char *device)
{
	char *buf = NULL;

	if (device) {
		buf = strdup(device);
		if (NULL == buf)
			return -ENOMEM;
	}

	if (__default_device)
		free(__default_device);

	__default_device = buf;

	return 0;
}

/*
 *
 */
static int set_default_baudrate(const char *baudrate)
{
	if (NULL == baudrate)
		return -EINVAL;

	__default_baudrate = __termbits(baudrate);

	return 0;
}

/*
 *
 */
static int device_traverse_trees(xmlNode *node)
{
	if (NULL == node)
		return -EINVAL;

	xmlNode *curr = node;

	for (; curr; curr = curr->next) {
		if (curr->type == XML_ELEMENT_NODE) {
			if (0 == xmlStrcmp(curr->name, (xmlChar*)"default")) {
				char *device = (char*)xmlGetProp(curr,
						(xmlChar*)"device");
				set_default_device(device);
				xmlFree(device);

				char *baudrate = (char*)xmlGetProp(curr,
						(xmlChar*)"baudrate");
					set_default_baudrate(baudrate);
				xmlFree(baudrate);
			} if (0 == xmlStrcmp(curr->name, (xmlChar*)"devices")) {
				int ret = __device_create_pool(
						(int)xmlChildElementCount(curr));
				if (0 > ret)
					break;
			} else if (0 == xmlStrcmp(curr->name,
						(xmlChar*)"usbdev")) {
				__device_add(curr);
			}
		}

		device_traverse_trees(curr->children);
	}

	return 0;
}

/*
 *
 */
static struct gpsdevice* gps_lookup(uint16_t vid, uint16_t pid)
{
	struct gpsdevice **devs = devices;

	for (; *devs; devs++) {
		if ((*devs)->vid == vid && (*devs)->pid == pid)
			return *devs;
	}

	return NULL;
}

static int usbdev_lookup(libusb_device **devs,
		char** _devname, speed_t *_baudrate)
{
	if ((NULL == _devname) || (NULL == _baudrate))
		return -EINVAL;

	libusb_device **dev = devs;
	for (; *dev; dev++) {
		struct libusb_device_descriptor desc;
		int ret = libusb_get_device_descriptor(*dev, &desc);
		if (ret < 0) {
			printf("failed to get device descriptor\n");
			continue;
		}

		struct gpsdevice* gps = gps_lookup(
				desc.idVendor, desc.idProduct);
		if (gps) {
			char path[PATH_MAX];

			snprintf(path, sizeof(path), "/dev/bus/usb/%03d/%03d",
					libusb_get_bus_number(*dev),
					libusb_get_device_address(*dev));
			*_devname = strdup(path);
			*_baudrate = gps->baudrate;
			return 0;
		}
	}

	return -EINVAL;
}

static int read_usb_gps_list(const char* filename)
{
	if (filename == NULL)
		return -EINVAL;

	xmlDoc *doc = xmlReadFile(filename, NULL, 0);
	if (doc == NULL)
		return -EINVAL;

	xmlNode *root = xmlDocGetRootElement(doc);
	if (root == NULL) {
		printf("empty document??\n");
		xmlFreeDoc(doc);
		return -EINVAL;
	}

	if (0 == xmlStrcmp(root->name, (xmlChar*)"odroid-gps")) {
		device_traverse_trees(root);
	}

	xmlFreeDoc(doc);
	xmlCleanupParser();

	if (0 <= nr_devices)
		printf("%d device(s) are listed\n", nr_devices);

	return nr_devices;
}

int scan_usb_gps_device(char **_devname, speed_t *_baudrate)
{
	int ret = -EINVAL;

	int nr = read_usb_gps_list(GPS_DEVICE_LIST_FILE);
	if (0 < nr) {
		libusb_init(NULL);

		libusb_device **devs;
		ssize_t sz = libusb_get_device_list(NULL, &devs);
		if (0 < sz)
			ret = usbdev_lookup(devs, _devname, _baudrate);

		libusb_free_device_list(devs, 1);
		libusb_exit(NULL);
	}

	__device_release_pool(devices);
	devices = NULL;

	set_default_device(NULL);

	return ret;
}

#if !defined(__ANDROID__)
int main(void)
{
	int count = 5;

	char *devname;
	speed_t baudrate;
	int ret;

	while (count--) {
		ret = scan_usb_gps_device(&devname, &baudrate);
		if (0 == ret) {
			printf("device name = %s, baudrate = B%s\n",
					devname, nameof_termbits(baudrate));
		}

		free(devname);
	}

	return ret;
}
#endif
