LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ALL_SRC := $(wildcard $(LOCAL_PATH)/src/*.c)
LOCAL_SRC_FILES := $(filter-out %_win32.c, $(patsubst $(LOCAL_PATH)/%, %, $(ALL_SRC)))
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include

LOCAL_CFLAGS += -O3 -Weverything
LOCAL_LDLIBS := -llog -landroid -lEGL -lGLESv3
LOCAL_MODULE := linear-congruency # important caveat, see NB below for details

include $(BUILD_SHARED_LIBRARY)