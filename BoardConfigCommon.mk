#
# Copyright (C) 2022 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

# Inherit from the proprietary version
include vendor/xiaomi/sm8450-common/BoardConfigVendor.mk

COMMON_PATH := device/xiaomi/sm8450-common

BUILD_BROKEN_DUP_RULES := true
BUILD_BROKEN_ELF_PREBUILT_PRODUCT_COPY_FILES := true

# A/B
AB_OTA_UPDATER := true

AB_OTA_PARTITIONS += \
    boot \
    dtbo \
    odm \
    product \
    recovery \
    system \
    system_ext \
    vbmeta \
    vbmeta_system \
    vendor \
    vendor_boot \
    vendor_dlkm

# Architecture
TARGET_ARCH := arm64
TARGET_ARCH_VARIANT := armv8-a-branchprot
TARGET_CPU_ABI := arm64-v8a
TARGET_CPU_ABI2 :=
TARGET_CPU_VARIANT := generic
TARGET_CPU_VARIANT_RUNTIME := kryo300

TARGET_2ND_ARCH := arm
TARGET_2ND_ARCH_VARIANT := armv8-2a
TARGET_2ND_CPU_ABI := armeabi-v7a
TARGET_2ND_CPU_ABI2 := armeabi
TARGET_2ND_CPU_VARIANT := generic
TARGET_2ND_CPU_VARIANT_RUNTIME := cortex-a75

# Audio
AUDIO_FEATURE_ENABLED_DLKM := true
AUDIO_FEATURE_ENABLED_DTS_EAGLE := false
AUDIO_FEATURE_ENABLED_GEF_SUPPORT := true
AUDIO_FEATURE_ENABLED_HW_ACCELERATED_EFFECTS := false
AUDIO_FEATURE_ENABLED_INSTANCE_ID := true
AUDIO_FEATURE_ENABLED_LSM_HIDL := true
AUDIO_FEATURE_ENABLED_PAL_HIDL := true
AUDIO_FEATURE_ENABLED_PROXY_DEVICE := true

TARGET_USES_QCOM_MM_AUDIO := true

# AVB
BOARD_AVB_ENABLE := true
BOARD_AVB_MAKE_VBMETA_IMAGE_ARGS += --flags 3
BOARD_MOVE_GSI_AVB_KEYS_TO_VENDOR_BOOT := true

BOARD_AVB_RECOVERY_KEY_PATH := external/avb/test/data/testkey_rsa2048.pem
BOARD_AVB_RECOVERY_ALGORITHM := SHA256_RSA2048
BOARD_AVB_RECOVERY_ROLLBACK_INDEX := $(PLATFORM_SECURITY_PATCH_TIMESTAMP)
BOARD_AVB_RECOVERY_ROLLBACK_INDEX_LOCATION := 1

BOARD_AVB_VBMETA_SYSTEM := system system_ext product
BOARD_AVB_VBMETA_SYSTEM_KEY_PATH := external/avb/test/data/testkey_rsa2048.pem
BOARD_AVB_VBMETA_SYSTEM_ALGORITHM := SHA256_RSA2048
BOARD_AVB_VBMETA_SYSTEM_ROLLBACK_INDEX := $(PLATFORM_SECURITY_PATCH_TIMESTAMP)
BOARD_AVB_VBMETA_SYSTEM_ROLLBACK_INDEX_LOCATION := 2

# Bootloader
TARGET_BOOTLOADER_BOARD_NAME := taro
TARGET_NO_BOOTLOADER := true

# Filesystem
TARGET_FS_CONFIG_GEN := $(COMMON_PATH)/configs/config.fs

# HIDL
DEVICE_MATRIX_FILE := $(COMMON_PATH)/hidl/compatibility_matrix.xml

DEVICE_MANIFEST_SKUS := taro
DEVICE_MANIFEST_TARO_FILES := \
    $(COMMON_PATH)/hidl/manifest_taro.xml \
    $(COMMON_PATH)/hidl/manifest_xiaomi.xml

DEVICE_FRAMEWORK_COMPATIBILITY_MATRIX_FILE := \
    $(COMMON_PATH)/hidl/vendor_framework_compatibility_matrix.xml \
    $(COMMON_PATH)/hidl/xiaomi_framework_compatibility_matrix.xml \
    vendor/lineage/config/device_framework_matrix.xml

# Kernel
# is-board-platform-in-list is used in split files below
# from https://git.codelinaro.org/clo/la/platform/vendor/qcom-opensource/core-utils/-/blob/LA.QSSI.12.0.r1-08700.03-qssi.0/build/utils.mk
include $(COMMON_PATH)/build/utils.mk

include $(COMMON_PATH)/kernel/kernel-platform-board.mk
include vendor/qcom/opensource/audio-kernel/audio_kernel_vendor_board.mk
include vendor/qcom/opensource/camera-kernel/board.mk
include vendor/qcom/opensource/dataipa/dataipa_dlkm_vendor_board.mk
include vendor/qcom/opensource/datarmnet-ext/datarmnet_ext_dlkm_vendor_board.mk
include vendor/qcom/opensource/datarmnet/datarmnet_dlkm_vendor_board.mk
include vendor/qcom/opensource/display-drivers/display_driver_board.mk
include vendor/qcom/opensource/eva-kernel/eva_kernel_board.mk
include vendor/qcom/opensource/mmrm-driver/mmrm_kernel_board.mk
include vendor/qcom/opensource/video-driver/video_kernel_board.mk

BOARD_INCLUDE_DTB_IN_BOOTIMG := true
BOARD_RAMDISK_USE_LZ4 := true
BOARD_USES_GENERIC_KERNEL_IMAGE := true

BOARD_KERNEL_BASE        := 0x00000000
BOARD_KERNEL_PAGESIZE    := 4096

BOARD_BOOT_HEADER_VERSION := 4
BOARD_MKBOOTIMG_ARGS := --header_version $(BOARD_BOOT_HEADER_VERSION)

BOARD_VENDOR_RAMDISK_FRAGMENTS := dlkm
BOARD_VENDOR_RAMDISK_FRAGMENT.dlkm.KERNEL_MODULE_DIRS := top

BOARD_KERNEL_CMDLINE := \
    video=vfb:640x400,bpp=32,memsize=3072000 \
    disable_dma32=on \
    bootinfo.fingerprint=$(LINEAGE_VERSION) \
    mtdoops.fingerprint=$(LINEAGE_VERSION)
BOARD_BOOTCONFIG := \
    androidboot.hardware=qcom \
    androidboot.memcg=1 \
    androidboot.usbcontroller=a600000.dwc3

# Lineage Health
TARGET_HEALTH_CHARGING_CONTROL_SUPPORTS_BYPASS := false

# Metadata
BOARD_USES_METADATA_PARTITION := true

# Partitions
BOARD_FLASH_BLOCK_SIZE := 0x020000 # (BOARD_KERNEL_PAGESIZE * 64)
BOARD_BOOTIMAGE_PARTITION_SIZE := 0x0C000000
BOARD_DTBOIMG_PARTITION_SIZE := 0x01800000
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 0x06400000
BOARD_SUPER_PARTITION_SIZE := 0x220000000
BOARD_USERDATAIMAGE_PARTITION_SIZE := 239033364480 # 0x37A77FB000
BOARD_VENDOR_BOOTIMAGE_PARTITION_SIZE := 0x06000000

BOARD_SUPER_PARTITION_GROUPS := qti_dynamic_partitions
BOARD_QTI_DYNAMIC_PARTITIONS_PARTITION_LIST := odm product system system_ext vendor vendor_dlkm
BOARD_QTI_DYNAMIC_PARTITIONS_SIZE := 9122611200 # 0x21FC00000 # BOARD_SUPER_PARTITION_SIZE - overhead (4MiB)

BOARD_ODMIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_PRODUCTIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_SYSTEMIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_SYSTEM_EXTIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_VENDORIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_VENDOR_DLKMIMAGE_FILE_SYSTEM_TYPE := ext4

TARGET_COPY_OUT_ODM := odm
TARGET_COPY_OUT_PRODUCT := product
TARGET_COPY_OUT_SYSTEM_EXT := system_ext
TARGET_COPY_OUT_VENDOR := vendor
TARGET_COPY_OUT_VENDOR_DLKM := vendor_dlkm

# Platform
BOARD_USES_QCOM_HARDWARE := true

# Power
TARGET_POWERHAL_MODE_EXT := $(COMMON_PATH)/power/power-mode.cpp

# PowerShare
SOONG_CONFIG_NAMESPACES += XIAOMI_POWERSHARE
SOONG_CONFIG_XIAOMI_POWERSHARE := WIRELESS_TX_ENABLE_PATH
SOONG_CONFIG_XIAOMI_POWERSHARE_WIRELESS_TX_ENABLE_PATH := /sys/class/qcom-battery/reverse_chg_mode

# Properties
TARGET_ODM_PROP += $(COMMON_PATH)/properties/odm.prop
TARGET_PRODUCT_PROP += $(COMMON_PATH)/properties/product.prop
TARGET_SYSTEM_PROP += $(COMMON_PATH)/properties/system.prop
TARGET_SYSTEM_EXT_PROP += $(COMMON_PATH)/properties/system_ext.prop
TARGET_VENDOR_PROP += $(COMMON_PATH)/properties/vendor.prop

# Recovery
#namespace definition for librecovery_updater
#differentiate legacy 'sg' or 'bsg' framework
SOONG_CONFIG_NAMESPACES += ufsbsg
SOONG_CONFIG_ufsbsg += ufsframework
SOONG_CONFIG_ufsbsg_ufsframework := bsg

TARGET_RECOVERY_FSTAB := $(COMMON_PATH)/rootdir/etc/recovery.fstab
TARGET_RECOVERY_PIXEL_FORMAT := RGBX_8888
TARGET_USERIMAGES_USE_EXT4 := true
TARGET_USERIMAGES_USE_F2FS := true
BOARD_EXCLUDE_KERNEL_FROM_RECOVERY_IMAGE := true

# RIL
ENABLE_VENDOR_RIL_SERVICE := true

# Sepolicy
include device/qcom/sepolicy_vndr/SEPolicy.mk

SYSTEM_EXT_PRIVATE_SEPOLICY_DIRS += $(COMMON_PATH)/sepolicy/private
SYSTEM_EXT_PUBLIC_SEPOLICY_DIRS += $(COMMON_PATH)/sepolicy/public
BOARD_VENDOR_SEPOLICY_DIRS += $(COMMON_PATH)/sepolicy/vendor

# WiFi
BOARD_HAS_QCOM_WLAN := true
BOARD_WLAN_DEVICE := qcwcn
BOARD_HOSTAPD_DRIVER := NL80211
BOARD_HOSTAPD_PRIVATE_LIB := lib_driver_cmd_$(BOARD_WLAN_DEVICE)
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_$(BOARD_WLAN_DEVICE)
QC_WIFI_HIDL_FEATURE_DUAL_AP := true
QC_WIFI_HIDL_FEATURE_DUAL_STA := true
WIFI_DRIVER_BUILT := qca_cld3
WIFI_DRIVER_DEFAULT := qca_cld3
WIFI_DRIVER_STATE_CTRL_PARAM := "/dev/wlan"
WIFI_DRIVER_STATE_OFF := "OFF"
WIFI_DRIVER_STATE_ON := "ON"
WIFI_HIDL_FEATURE_AWARE := true
WIFI_HIDL_FEATURE_DUAL_INTERFACE := true
WIFI_HIDL_UNIFIED_SUPPLICANT_SERVICE_RC_ENTRY := true
WPA_SUPPLICANT_VERSION := VER_0_8_X

PRODUCT_VENDOR_MOVE_ENABLED := true
WIFI_DRIVER_INSTALL_TO_KERNEL_OUT := true
ifneq ($(TARGET_WLAN_CHIP),)
	BOARD_VENDOR_KERNEL_MODULES += $(foreach chip, $(TARGET_WLAN_CHIP), $(KERNEL_MODULES_OUT)/$(WLAN_CHIPSET)_$(chip).ko)
else
	BOARD_VENDOR_KERNEL_MODULES += $(KERNEL_MODULES_OUT)/qca_cld3_wlan.ko
endif

BOARD_WPA_SUPPLICANT_PRIVATE_LIB_EVENT := "ON"
BOARD_HOSTAPD_PRIVATE_LIB_EVENT := "ON"
