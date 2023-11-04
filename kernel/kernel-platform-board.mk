ifeq ($(TARGET_USES_KERNEL_PLATFORM),true)

ifeq ($(wildcard $(KERNEL_PREBUILT_DIR)/),)
$(warning $(KERNEL_PREBUILT_DIR) does not exist and TARGET_USES_KERNEL_PLATFORM=$(TARGET_USES_KERNEL_PLATFORM))
else
################################################################################
# UAPI headers
TARGET_BOARD_KERNEL_HEADERS := $(KERNEL_PREBUILT_DIR)/kernel-headers

################################################################################
# Path to unsigned abl image
TARGET_BOARD_UNSIGNED_ABL_DIR := $(KERNEL_PREBUILT_DIR)/kernel-abl/abl-$(TARGET_BUILD_VARIANT)

################################################################################
# DTBs
ifneq (,$(wildcard $(KERNEL_PREBUILT_DIR)/dtbs/))
BOARD_PREBUILT_DTBOIMAGE := $(KERNEL_PREBUILT_DIR)/dtbs/dtbo.img
# Path to prebuilt .dtb's used for dtb.img generation
BOARD_PREBUILT_DTBIMAGE_DIR := $(KERNEL_PREBUILT_DIR)/dtbs/
else
BOARD_PREBUILT_DTBOIMAGE := $(KERNEL_PREBUILT_DIR)/dtbo.img
# Path to prebuilt .dtb's used for dtb.img generation
BOARD_PREBUILT_DTBIMAGE_DIR := $(KERNEL_PREBUILT_DIR)/
endif

################################################################################
# DLKMs
# get-kernel-modules:
# $1: directory inside KERNEL_PREBUILT_DIR to find the modules
define get-kernel-modules
$(if $(wildcard $(KERNEL_PREBUILT_DIR)/$(1)/modules.load), \
	$(addprefix $(KERNEL_PREBUILT_DIR)/$(1)/,$(notdir $(file < $(KERNEL_PREBUILT_DIR)/$(1)/modules.load))), \
	$(wildcard $(KERNEL_PREBUILT_DIR)/$(1)/*.ko))
endef

prepend-kernel-modules = $(eval $1 := $2 $(filter-out $2,$($1)))

first_stage_modules := $(call get-kernel-modules,.)
second_stage_modules := $(call get-kernel-modules,vendor_dlkm)

BOARD_VENDOR_RAMDISK_KERNEL_MODULES += $(first_stage_modules)
BOARD_VENDOR_RAMDISK_KERNEL_MODULES += $(second_stage_modules)

$(call prepend-kernel-modules,BOARD_VENDOR_RAMDISK_KERNEL_MODULES_LOAD,$(first_stage_modules))
$(call prepend-kernel-modules,BOARD_VENDOR_RAMDISK_RECOVERY_KERNEL_MODULES_LOAD,$(first_stage_modules) $(second_stage_modules))

$(call prepend-kernel-modules,BOARD_VENDOR_KERNEL_MODULES,$(second_stage_modules))

BOARD_VENDOR_KERNEL_MODULES_BLOCKLIST_FILE := $(wildcard $(KERNEL_PREBUILT_DIR)/vendor_dlkm/modules.blocklist)
# vendor ramdisk has second-stage modules for recovery
BOARD_VENDOR_RAMDISK_KERNEL_MODULES_BLOCKLIST_FILE := $(wildcard $(KERNEL_PREBUILT_DIR)/vendor_dlkm/modules.blocklist)

endif # ($(wildcard $(KERNEL_PREBUILT_DIR)/),)

endif # ($(TARGET_USES_KERNEL_PLATFORM),true)
