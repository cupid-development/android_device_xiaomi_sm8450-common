KERNEL_PREBUILT_DEBUG := device/qcom/$(PRODUCT_DEVICE)-kernel/debug
LOCAL_PATH := $(KERNEL_PREBUILT_DEBUG)
KERNEL_DEBUG_FILES := $(if $(wildcard $(LOCAL_PATH)/),$(call find-subdir-files,-type f))

kernel-debug-src-to-module = $(foreach f,$1,kernel-debug-$(subst /,-,$f))

define add-kernel-debug-file
include $(CLEAR_VARS)
LOCAL_MODULE := $(call kernel-debug-src-to-module,$1)
LOCAL_SRC_FILES := $1
LOCAL_MODULE_RELATIVE_PATH := $(dir $1)
LOCAL_MODULE_STEM := $(notdir $1)
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)
LOCAL_MODULE_OWNER := qti
include $(BUILD_PREBUILT)
endef

$(foreach f,$(KERNEL_DEBUG_FILES),$(eval $(call add-kernel-debug-file,$f)))
KERNEL_DEBUG_MODULES := $(call kernel-debug-src-to-module,$(KERNEL_DEBUG_FILES))

include $(CLEAR_VARS)
LOCAL_MODULE := kernel-platform-debug-files
LOCAL_REQUIRED_MODULES := $(KERNEL_DEBUG_MODULES)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := qti
include $(BUILD_PHONY_PACKAGE)
