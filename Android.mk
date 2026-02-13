CAMERA_DLKM_ENABLED := true
ifeq ($(TARGET_KERNEL_DLKM_DISABLE), true)
	ifeq ($(TARGET_KERNEL_DLKM_CAMERA_OVERRIDE), false)
		CAMERA_DLKM_ENABLED := false;
	endif
endif

ifeq ($(CAMERA_DLKM_ENABLED),true)
ifeq ($(call is-board-platform-in-list, $(TARGET_BOARD_PLATFORM)),true)

# Make target to specify building the camera.ko from within Android build system.
LOCAL_PATH := $(call my-dir)
# Path to DLKM make scripts
DLKM_DIR := $(TOP)/device/qcom/common/dlkm

LOCAL_MODULE_DDK_BUILD := true
ifneq ($(TARGET_BOARD_PLATFORM),)
LOCAL_MODULE_DDK_EXTRA_ARGS := "--//vendor/qcom/opensource/camera-kernel:project_name=$(TARGET_BOARD_PLATFORM)"
endif



# Kbuild options
KBUILD_OPTIONS := CAMERA_KERNEL_ROOT=$(CURDIR)/$(LOCAL_PATH)
KBUILD_OPTIONS += KERNEL_ROOT=$(CURDIR)/kernel/msm-$(TARGET_KERNEL_VERSION)/
KBUILD_OPTIONS += MODNAME=camera
KBUILD_OPTIONS += BOARD_PLATFORM=$(TARGET_BOARD_PLATFORM)

# Clear shell environment variables from previous android module during build
include $(CLEAR_VARS)
# For incremental compilation support.
LOCAL_SRC_FILES := \
            $(call all-subdir-files,config) \
            $(call all-subdir-files,camera/drivers) \
            $(call all-subdir-files,camera/dt-bindings) \
            $(call all-subdir-files,camera/include)     \
            $(call all-subdir-files,common)             \
            $(LOCAL_PATH)/Android.mk \
            $(LOCAL_PATH)/board.mk   \
            $(LOCAL_PATH)/product.mk \
            $(LOCAL_PATH)/Kbuild

LOCAL_MODULE_PATH           := $(KERNEL_MODULES_OUT)
LOCAL_MODULE                := camera.ko
LOCAL_MODULE_TAGS           := optional
#LOCAL_MODULE_KBUILD_NAME   := camera.ko
#LOCAL_MODULE_DEBUG_ENABLE  := true

ifneq (, $(filter $(TARGET_BOARD_PLATFORM), shikra))
# Use DDK build system for shikra and holi platforms
include $(DLKM_DIR)/Build_external_kernelmodule.mk
# Include Camera UAPI Android.mk target to copy headers
else
# Use legacy build system for other platforms
include $(DLKM_DIR)/AndroidKernelModule.mk
# Include Camera UAPI Android.mk target to copy headers
include $(LOCAL_PATH)/include/uapi/Android.mk
endif

endif # End of check for board platform
endif # ifeq ($(CAMERA_DLKM_ENABLED),true)
