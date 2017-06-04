# Use hardware GPS implementation if available.
#

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(BOARD_HAVE_ODROID_GPS), true)

ifeq ($(BOARD_SUPPORT_EXTERNAL_GPS), true)
LOCAL_CFLAGS += -DEXTERNAL_GPS
endif

LOCAL_MODULE := gps.$(TARGET_PRODUCT)
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	odroid_gps.c \
	usbdevice.c \
	nmea_reader.h \
	nmea_reader.c \
	nmea_tokenizer.h \
	nmea_tokenizer.c

LOCAL_SHARED_LIBRARIES := \
	libusb-1.0 \
	libxml2 \
	libutils \
	libcutils \
	libdl \
	libc

LOCAL_CFLAGS += -DANDROID -Wall -Wextra

LOCAL_C_INCLUDES += \
	external/libxml2/include \
	external/icu/icu4c/source/common \
	external/libusb/libusb

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

include $(BUILD_SHARED_LIBRARY)

endif
