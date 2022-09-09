
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.gnss-aidl-impl-qti
# activate the following line for debug purposes only, comment out for production
#LOCAL_SANITIZE_DIAG += $(GNSS_SANITIZE_DIAG)

LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_VINTF_FRAGMENTS := android.hardware.gnss-aidl-service-qti.xml

LOCAL_SRC_FILES := \
    Gnss.cpp \
    GnssConfiguration.cpp \
    GnssPowerIndication.cpp \
    GnssMeasurementInterface.cpp \
    location_api/GnssAPIClient.cpp

LOCAL_HEADER_LIBRARIES := \
    libgps.utils_headers \
    libloc_core_headers \
    libloc_pla_headers \
    liblocation_api_headers

LOCAL_SHARED_LIBRARIES := \
    libbase \
    libbinder_ndk \
    android.hardware.gnss-V1-ndk \
    liblog \
    libcutils \
    libutils \
    libloc_core \
    libgps.utils \
    libdl \
    liblocation_api

ifneq ($(TARGET_SUPPORTS_WEARABLES),true)
    LOCAL_SHARED_LIBRARIES += libqti_vndfwk_detect_vendor
else
    LOCAL_SHARED_LIBRARIES += libqti_vndfwk_detect
endif

LOCAL_CFLAGS += $(GNSS_CFLAGS)

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.gnss-aidl-service-qti
# activate the following line for debug purposes only, comment out for production
#LOCAL_SANITIZE_DIAG += $(GNSS_SANITIZE_DIAG)
LOCAL_VINTF_FRAGMENTS := android.hardware.gnss-aidl-service-qti.xml
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_INIT_RC := android.hardware.gnss-aidl-service-qti.rc
LOCAL_SRC_FILES := \
    service.cpp

LOCAL_HEADER_LIBRARIES := \
    libgps.utils_headers \
    libloc_core_headers \
    libloc_pla_headers \
    liblocation_api_headers

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libcutils \
    libdl \
    libbase \
    libutils \
    libgps.utils \
    liblocation_api \
    libbinder_ndk

ifneq ($(TARGET_SUPPORTS_WEARABLES),true)
    LOCAL_SHARED_LIBRARIES += libqti_vndfwk_detect_vendor
else
    LOCAL_SHARED_LIBRARIES += libqti_vndfwk_detect
endif

LOCAL_SHARED_LIBRARIES += \
    libhidlbase \
    android.hardware.gnss@1.0 \
    android.hardware.gnss@1.1 \
    android.hardware.gnss@2.0 \
    android.hardware.gnss@2.1 \
    android.hardware.gnss-V1-ndk \
    android.hardware.gnss-aidl-impl-qti

LOCAL_CFLAGS += $(GNSS_CFLAGS)

include $(BUILD_EXECUTABLE)
