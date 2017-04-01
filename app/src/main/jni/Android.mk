LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_LDLIBS := -lm -llog

LOCAL_MODULE := MyLibrary
LOCAL_SRC_FILES =: MyLibrary.cpp
include $(BUILD_SHARED_LIBRARY)