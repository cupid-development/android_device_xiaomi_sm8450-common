# HAL packages
ifneq ($(BOARD_VENDOR_QCOM_GPS_LOC_API_HARDWARE),)

PRODUCT_PACKAGES += gps.conf
PRODUCT_PACKAGES += batching.conf
PRODUCT_PACKAGES += gnss_antenna_info.conf
PRODUCT_PACKAGES += gnss@2.0-base.policy
PRODUCT_PACKAGES += gnss@2.0-xtra-daemon.policy
PRODUCT_PACKAGES += gnss@2.0-xtwifi-client.policy
PRODUCT_PACKAGES += libloc_pla_headers
PRODUCT_PACKAGES += liblocation_api_headers
PRODUCT_PACKAGES += libgps.utils_headers
PRODUCT_PACKAGES += liblocation_api
PRODUCT_PACKAGES += libgps.utils
PRODUCT_PACKAGES += libbatching
PRODUCT_PACKAGES += libgeofencing
PRODUCT_PACKAGES += libloc_core
PRODUCT_PACKAGES += libgnss

ifeq ($(strip $(TARGET_BOARD_AUTO)),true)
PRODUCT_PACKAGES += libgnssauto_power
endif #TARGET_BOARD_AUTO

PRODUCT_PACKAGES += android.hardware.gnss@2.1-impl-qti
PRODUCT_PACKAGES += android.hardware.gnss-aidl-impl-qti
PRODUCT_PACKAGES += android.hardware.gnss-aidl-service-qti

## Feature flags - self contained FR in gps module
# Enable NHz location feature. Default is false.
# Set this flag to true to enable the NHz location feature.
FEATURE_LOCATION_NHZ := false

# Soong Namespace
SOONG_CONFIG_NAMESPACES += qtilocation

# Soong Keys
SOONG_CONFIG_qtilocation := feature_nhz

# Soong Values
SOONG_CONFIG_qtilocation_feature_nhz := false

# Enable NHz location feature
ifeq ($(FEATURE_LOCATION_NHZ),true)
    SOONG_CONFIG_qtilocation_feature_nhz := true
endif

endif # ifneq ($(BOARD_VENDOR_QCOM_GPS_LOC_API_HARDWARE),)
