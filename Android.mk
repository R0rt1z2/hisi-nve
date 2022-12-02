LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := hisi-nve

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/src/nve/include \
    $(LOCAL_PATH)/src/crypto/include \
    $(LOCAL_PATH)/src/memory/include

LOCAL_SRC_FILES := \
    src/nve/hisi_nve.c \
    src/crypto/sha256.c \
    src/memory/memory.c

LOCAL_CFLAGS += \
    -Wno-pointer-sign \
    -Wno-int-to-pointer-cast

include $(BUILD_EXECUTABLE)

$(call import-add-path, $(LOCAL_PATH))
