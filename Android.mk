LOCAL_PATH := $(call my-dir)
ifeq ($(call is-vendor-board-platform,QCOM),true)

# HAL module implemenation stored in
# hw/<POWERS_HARDWARE_MODULE_ID>.<ro.hardware>.so
include $(CLEAR_VARS)
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SHARED_LIBRARIES := liblog libcutils libdl libxml2 libbase libutils libbinder_ndk
ifeq ($(call math_gt_or_eq, 33, $(PLATFORM_SDK_VERSION)), true)
    LOCAL_SHARED_LIBRARIES += android.hardware.power-V1-ndk
else
    LOCAL_SHARED_LIBRARIES += android.hardware.power-V1-ndk_platform
endif
LOCAL_HEADER_LIBRARIES += libutils_headers
LOCAL_HEADER_LIBRARIES += libhardware_headers
LOCAL_SRC_FILES := power-common.c metadata-parser.c utils.c list.c hint-data.c powerhintparser.c Power.cpp main.cpp
LOCAL_C_INCLUDES := external/libxml2/include \
                    external/icu/icu4c/source/common

ifeq ($(call is-board-platform-in-list,msm8937), true)
LOCAL_SRC_FILES += power-8937.c
endif

ifeq ($(call is-board-platform-in-list,monaco), true)
LOCAL_SRC_FILES += power-monaco.c
endif

ifeq ($(TARGET_USES_INTERACTION_BOOST),true)
    LOCAL_CFLAGS += -DINTERACTION_BOOST
endif
LOCAL_MODULE := android.hardware.power-service
LOCAL_INIT_RC := android.hardware.power-service.rc
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS += -Wno-unused-parameter -Wno-unused-variable
LOCAL_VENDOR_MODULE := true
LOCAL_VINTF_FRAGMENTS := power.xml
include $(BUILD_EXECUTABLE)
endif
