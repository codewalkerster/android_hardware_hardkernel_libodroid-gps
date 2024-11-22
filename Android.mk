# Copyright (C) 2011-2014 The Android-x86 Open Source Project

LOCAL_PATH := $(call my-dir)

# HAL module implemenation, not prelinked and stored in
# hw/<OVERLAY_HARDWARE_MODULE_ID>.<ro.product.board>.so
include $(CLEAR_VARS)

LOCAL_C_INCLUDES += \
	hardware/libhardware/include \
	system/core/libcutils/include \
	system/core/libutils/include \
	system/core/libsystem/include

LOCAL_CFLAGS += -DANDROID -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-sign-compare -Wno-unused-comparison -Wno-incompatible-pointer-types -Wno-unused-function -Wno-format
LOCAL_SHARED_LIBRARIES := liblog libcutils
LOCAL_MODULE := gps.amlogic
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := gps.c
LOCAL_PRELINK_MODULE := false
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := hw

include $(BUILD_SHARED_LIBRARY)
