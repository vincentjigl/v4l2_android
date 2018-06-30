LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

TARGET_PATH=$(TOP)/frameworks/av/media/libcedarx/demo/demoVencoder
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES:= main.c mjpeg.c

LOCAL_C_INCLUDES := \
	$(TARGET_PATH) \
	$(TARGET_PATH)/../../ \
	$(TARGET_PATH)/../../libcore/base/include/      \
	$(TARGET_PATH)/../../CODEC/VIDEO/ENCODER/include/ \
	$(TARGET_PATH)/../../libcore/playback/include  \
	$(TARGET_PATH)/../../external/include/adecoder \
	$(TOP)/frameworks/av/media/libcedarc/include \


LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	libVE \
	libMemAdapter \
	libvencoder \
	libvdecoder \
    libcdx_stream   \
    libcdx_parser   \
    libcdx_common   \
	libcdx_base

LOCAL_MODULE:= vcam

#include $(BUILD_SHARED_LIBRARY)
include $(BUILD_EXECUTABLE)
