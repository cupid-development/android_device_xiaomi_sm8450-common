#
# Copyright (C) 2024 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

from typing import List


from extract_utils.main import (
    ExtractUtilsModule,
)

from extract_utils.fixups_blob import (
    blob_fixups_user_type,
    blob_fixup,
)

from extract_utils.fixups_lib import (
    lib_fixups_user_type,
    lib_fixup_remove_arch_suffix,
    lib_fixup_vendorcompat,
    libs_clang_rt_ubsan,
    libs_proto_3_9_1,
)


namespace_imports = [
    'device/xiaomi/sm8450-common',
    'hardware/qcom-caf/sm8450',
    'hardware/qcom-caf/wlan',
    'hardware/xiaomi',
    'vendor/qcom/opensource/commonsys/display',
	'vendor/qcom/opensource/commonsys-intf/display',
    'vendor/qcom/opensource/dataservices',
]


libs_add_vendor_suffix = (
    'vendor.qti.hardware.dpmservice@1.0',
    'vendor.qti.hardware.dpmservice@1.1',
    'vendor.qti.hardware.qccsyshal@1.0',
    'vendor.qti.hardware.qccsyshal@1.1',
    'vendor.qti.hardware.qccvndhal@1.0',
    'vendor.qti.imsrtpservice@3.0',
    'vendor.qti.diaghal@1.0',
    'vendor.qti.hardware.wifidisplaysession@1.0',
    'com.qualcomm.qti.dpm.api@1.0',
)

libs_remove = (
    'libagm',
    'libar-pal',
    'libpalclient',
    'libwpa_client',
)


def lib_fixup_vendor_suffix(lib: str, partition: str, *args, **kwargs):
    if partition != 'vendor':
        return None

    return f'{lib}-{partition}'


def lib_fixup_remove(lib: str, *args, **kwargs):
    return ''


lib_fixups: lib_fixups_user_type = {
    libs_clang_rt_ubsan: lib_fixup_remove_arch_suffix,
    libs_proto_3_9_1: lib_fixup_vendorcompat,
    libs_add_vendor_suffix: lib_fixup_vendor_suffix,
    libs_remove: lib_fixup_remove,
}


blob_fixups: blob_fixups_user_type = {
    'system_ext/lib64/libwfdservice.so': blob_fixup().replace_needed(
        'android.media.audio.common.types-V2-cpp.so',
        'android.media.audio.common.types-V3-cpp.so',
    ),
    (
        'vendor/bin/hw/android.hardware.security.keymint-service-qti',
        'vendor/lib64/libqtikeymint.so',
    ): blob_fixup()
    .replace_needed(
        'android.hardware.security.keymint-V1-ndk_platform.so',
        'android.hardware.security.keymint-V1-ndk.so',
    )
    .replace_needed(
        'android.hardware.security.secureclock-V1-ndk_platform.so',
        'android.hardware.security.secureclock-V1-ndk.so',
    )
    .replace_needed(
        'android.hardware.security.sharedsecret-V1-ndk_platform.so',
        'android.hardware.security.sharedsecret-V1-ndk.so',
    )
    .add_needed('android.hardware.security.rkp-V1-ndk.so'),
    'vendor/bin/qcc-trd': blob_fixup().replace_needed(
        'libgrpc++_unsecure.so', 'libgrpc++_unsecure_prebuilt.so'
    ),
    'vendor/etc/media_codecs_c2_audio.xml': blob_fixup().regex_replace('.+media_codecs_dolby_audio.+\n', ''),
    (
       'vendor/etc/media_codecs_cape.xml',
       'vendor/etc/media_codecs_diwali_v0.xml',
       'vendor/etc/media_codecs_diwali_v1.xml',
       'vendor/etc/media_codecs_diwali_v2.xml',
       'vendor/etc/media_codecs_taro.xml',
       'vendor/etc/media_codecs_ukee.xml',
    ): blob_fixup().regex_replace('.+media_codecs_(google_audio|google_c2|google_telephony|vendor_audio).+\n', ''),
    'vendor/etc/vintf/manifest/c2_manifest_vendor.xml': blob_fixup().regex_replace('.+dolby.+\n', ''),
}


module = ExtractUtilsModule(
    'sm8450-common',
    'xiaomi',
    blob_fixups=blob_fixups,
    lib_fixups=lib_fixups,
    namespace_imports=namespace_imports,
    check_elf=True,
)
