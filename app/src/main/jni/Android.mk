LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

OPENCV_INSTALL_MODULES:=on
OPENCV_LIB_TYPE:=SHARED

include ../../../../native/jni/OpenCV.mk

LOCAL_MODULE := OpenCV

LOCAL_SRC_FILES := my_fin_scanner.cpp

LOCAL_LDLIBS +=  -lm -llog -landroid

LOCAL_LDFLAGS += -ljnigraphics

include $(BUILD_SHARED_LIBRARY)