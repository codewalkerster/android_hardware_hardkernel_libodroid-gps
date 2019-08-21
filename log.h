/*
 * Copyright 2010 Ericsson AB
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Indrek Peri <Indrek.Peri@ericsson.com>  */

#ifndef _LIBMBMGPS_LOG_H
#define _LIBMBMGPS_LOG_H 1

#define  LOG_TAG  "libodroid-gps"
#include <cutils/log.h>

#define ENTER ALOGV("%s: enter", __FUNCTION__)
#define EXIT ALOGV("%s: exit", __FUNCTION__)

//#define DEBUG 1

#ifdef DEBUG
#  define  D(...)   ALOGD(__VA_ARGS__)
#else
#  define  D(...)   ((void)0)
#endif

#endif				/* end _LIBMBMGPS_LOG_H_ */
