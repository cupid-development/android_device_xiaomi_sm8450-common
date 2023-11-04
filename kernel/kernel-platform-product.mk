TARGET_USES_KERNEL_PLATFORM ?= true
ifeq ($(TARGET_USES_KERNEL_PLATFORM),true)
KERNEL_PREBUILT_DIR ?= device/qcom/$(shell echo -n $(TARGET_PRODUCT) | sed -e 's/^[a-z]*_//g')-kernel
KERNEL_PRODUCT_DIR := kernel_obj
KERNEL_MODULES_INSTALL := dlkm
KERNEL_MODULES_OUT ?= $(PRODUCT_OUT)/$(KERNEL_MODULES_INSTALL)/lib/modules

ifeq ($(wildcard $(KERNEL_PREBUILT_DIR)/),)
$(warning $(KERNEL_PREBUILT_DIR) does not exist and TARGET_USES_KERNEL_PLATFORM=$(TARGET_USES_KERNEL_PLATFORM))
else


PRODUCT_COPY_FILES += $(KERNEL_PREBUILT_DIR)/Image:kernel
PRODUCT_COPY_FILES += $(KERNEL_PREBUILT_DIR)/System.map:$(KERNEL_PRODUCT_DIR)/System.map

PRODUCT_PACKAGES_DEBUG += kernel-platform-debug-files

endif # ($(wildcard $(KERNEL_PREBUILT_DIR)/),)
endif # ($(TARGET_USES_KERNEL_PLATFORM),true)
