/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
 * Not a Contribution
 */
/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2_0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2_0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "LocSvc_GnssInterface"
#define LOG_NDEBUG 0

#include "Gnss.h"
#include <log_util.h>
#include <android/binder_auto_utils.h>

namespace android {
namespace hardware {
namespace gnss {
namespace aidl {
namespace implementation {

void gnssCallbackDied(void* cookie) {
    LOC_LOGe("IGnssCallback AIDL service died");
    Gnss* iface = static_cast<Gnss*>(cookie);
    if (iface != nullptr) {
        iface->handleClientDeath();
    }
}
ndk::ScopedAStatus Gnss::setCallback(const std::shared_ptr<IGnssCallback>& callback) {
    if (callback == nullptr) {
        LOC_LOGe("Null callback ignored");
        return ndk::ScopedAStatus::fromExceptionCode(STATUS_INVALID_OPERATION);
    }

    mGnssCallback = callback;
    GnssAPIClient* api = getApi();

    if (mGnssCallback != nullptr) {
        AIBinder_DeathRecipient* recipient = AIBinder_DeathRecipient_new(&gnssCallbackDied);
        AIBinder_linkToDeath(callback->asBinder().get(), recipient, this);
    }

    if (api != nullptr) {
        api->gnssUpdateCallbacks(callback);
        api->gnssEnable(LOCATION_TECHNOLOGY_TYPE_GNSS);
        api->requestCapabilities();
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Gnss::close() {
    if (mApi != nullptr) {
        mApi->gnssDisable();
    }
    return ndk::ScopedAStatus::ok();
}

Gnss::Gnss(): mGnssCallback(nullptr) {
    memset(&mPendingConfig, 0, sizeof(GnssConfig));
    ENTRY_LOG_CALLFLOW();
}

Gnss::~Gnss() {
    ENTRY_LOG_CALLFLOW();
    if (mApi != nullptr) {
        mApi->destroy();
        mApi = nullptr;
    }
}

void Gnss::handleClientDeath() {
    close();
    if (mApi != nullptr) {
        mApi->gnssUpdateCallbacks(mGnssCallback);
    }
    mGnssCallback = nullptr;
}

ndk::ScopedAStatus Gnss::updateConfiguration(GnssConfig& gnssConfig) {
    ENTRY_LOG_CALLFLOW();
    GnssAPIClient* api = getApi();
    if (api) {
        api->gnssConfigurationUpdate(gnssConfig);
    } else if (gnssConfig.flags != 0) {
        // api is not ready yet, update mPendingConfig with gnssConfig
        mPendingConfig.size = sizeof(GnssConfig);

        if (gnssConfig.flags & GNSS_CONFIG_FLAGS_GPS_LOCK_VALID_BIT) {
            mPendingConfig.flags |= GNSS_CONFIG_FLAGS_GPS_LOCK_VALID_BIT;
            mPendingConfig.gpsLock = gnssConfig.gpsLock;
        }
        if (gnssConfig.flags & GNSS_CONFIG_FLAGS_SUPL_VERSION_VALID_BIT) {
            mPendingConfig.flags |= GNSS_CONFIG_FLAGS_SUPL_VERSION_VALID_BIT;
            mPendingConfig.suplVersion = gnssConfig.suplVersion;
        }
        if (gnssConfig.flags & GNSS_CONFIG_FLAGS_SET_ASSISTANCE_DATA_VALID_BIT) {
            mPendingConfig.flags |= GNSS_CONFIG_FLAGS_SET_ASSISTANCE_DATA_VALID_BIT;
            mPendingConfig.assistanceServer.size = sizeof(GnssConfigSetAssistanceServer);
            mPendingConfig.assistanceServer.type = gnssConfig.assistanceServer.type;
            if (mPendingConfig.assistanceServer.hostName != nullptr) {
                free((void*)mPendingConfig.assistanceServer.hostName);
                mPendingConfig.assistanceServer.hostName =
                    strdup(gnssConfig.assistanceServer.hostName);
            }
            mPendingConfig.assistanceServer.port = gnssConfig.assistanceServer.port;
        }
        if (gnssConfig.flags & GNSS_CONFIG_FLAGS_LPP_PROFILE_VALID_BIT) {
            mPendingConfig.flags |= GNSS_CONFIG_FLAGS_LPP_PROFILE_VALID_BIT;
            mPendingConfig.lppProfileMask = gnssConfig.lppProfileMask;
        }
        if (gnssConfig.flags & GNSS_CONFIG_FLAGS_LPPE_CONTROL_PLANE_VALID_BIT) {
            mPendingConfig.flags |= GNSS_CONFIG_FLAGS_LPPE_CONTROL_PLANE_VALID_BIT;
            mPendingConfig.lppeControlPlaneMask = gnssConfig.lppeControlPlaneMask;
        }
        if (gnssConfig.flags & GNSS_CONFIG_FLAGS_LPPE_USER_PLANE_VALID_BIT) {
            mPendingConfig.flags |= GNSS_CONFIG_FLAGS_LPPE_USER_PLANE_VALID_BIT;
            mPendingConfig.lppeUserPlaneMask = gnssConfig.lppeUserPlaneMask;
        }
        if (gnssConfig.flags & GNSS_CONFIG_FLAGS_AGLONASS_POSITION_PROTOCOL_VALID_BIT) {
            mPendingConfig.flags |= GNSS_CONFIG_FLAGS_AGLONASS_POSITION_PROTOCOL_VALID_BIT;
            mPendingConfig.aGlonassPositionProtocolMask = gnssConfig.aGlonassPositionProtocolMask;
        }
        if (gnssConfig.flags & GNSS_CONFIG_FLAGS_EM_PDN_FOR_EM_SUPL_VALID_BIT) {
            mPendingConfig.flags |= GNSS_CONFIG_FLAGS_EM_PDN_FOR_EM_SUPL_VALID_BIT;
            mPendingConfig.emergencyPdnForEmergencySupl = gnssConfig.emergencyPdnForEmergencySupl;
        }
        if (gnssConfig.flags & GNSS_CONFIG_FLAGS_SUPL_EM_SERVICES_BIT) {
            mPendingConfig.flags |= GNSS_CONFIG_FLAGS_SUPL_EM_SERVICES_BIT;
            mPendingConfig.suplEmergencyServices = gnssConfig.suplEmergencyServices;
        }
        if (gnssConfig.flags & GNSS_CONFIG_FLAGS_SUPL_MODE_BIT) {
            mPendingConfig.flags |= GNSS_CONFIG_FLAGS_SUPL_MODE_BIT;
            mPendingConfig.suplModeMask = gnssConfig.suplModeMask;
        }
        if (gnssConfig.flags & GNSS_CONFIG_FLAGS_BLACKLISTED_SV_IDS_BIT) {
            mPendingConfig.flags |= GNSS_CONFIG_FLAGS_BLACKLISTED_SV_IDS_BIT;
            mPendingConfig.blacklistedSvIds = gnssConfig.blacklistedSvIds;
        }
        if (gnssConfig.flags & GNSS_CONFIG_FLAGS_EMERGENCY_EXTENSION_SECONDS_BIT) {
            mPendingConfig.flags |= GNSS_CONFIG_FLAGS_EMERGENCY_EXTENSION_SECONDS_BIT;
            mPendingConfig.emergencyExtensionSeconds = gnssConfig.emergencyExtensionSeconds;
        }
    }
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Gnss::getExtensionGnssConfiguration(
        std::shared_ptr<::aidl::android::hardware::gnss::IGnssConfiguration>* _aidl_return) {
    ENTRY_LOG_CALLFLOW();
    if (mGnssConfiguration == nullptr) {
        mGnssConfiguration = SharedRefBase::make<GnssConfiguration>(this);
    }
    *_aidl_return = mGnssConfiguration;
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Gnss::getExtensionGnssPowerIndication(
        std::shared_ptr<::aidl::android::hardware::gnss::IGnssPowerIndication>* _aidl_return) {
    ENTRY_LOG_CALLFLOW();
    if (mGnssPowerIndication == nullptr) {
        mGnssPowerIndication = SharedRefBase::make<GnssPowerIndication>();
    }
    *_aidl_return = mGnssPowerIndication;
    return ndk::ScopedAStatus::ok();
}
::ndk::ScopedAStatus Gnss::getExtensionGnssMeasurement(
        std::shared_ptr<::aidl::android::hardware::gnss::IGnssMeasurementInterface>* _aidl_return) {
    ENTRY_LOG_CALLFLOW();
    if (mGnssMeasurementInterface == nullptr) {
        mGnssMeasurementInterface = SharedRefBase::make<GnssMeasurementInterface>();
    }
    *_aidl_return = mGnssMeasurementInterface;
    return ndk::ScopedAStatus::ok();
}

GnssAPIClient* Gnss::getApi() {
    if (mApi != nullptr) {
        return mApi;
    }

    if (mGnssCallback != nullptr) {
        mApi = new GnssAPIClient(mGnssCallback);
    } else {
        LOC_LOGW("%s] GnssAPIClient is not ready", __FUNCTION__);
        return mApi;
    }

    if (mPendingConfig.size == sizeof(GnssConfig)) {
        // we have pending GnssConfig
        mApi->gnssConfigurationUpdate(mPendingConfig);
        // clear size to invalid mPendingConfig
        mPendingConfig.size = 0;
        if (mPendingConfig.assistanceServer.hostName != nullptr) {
            free((void*)mPendingConfig.assistanceServer.hostName);
        }
    }

    return mApi;
}
}  // namespace implementation
}  // namespace aidl
}  // namespace gnss
}  // namespace hardware
}  // namespace android
