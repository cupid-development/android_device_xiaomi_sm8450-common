/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
 * Not a Contribution
 */
/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "GnssPowerIndicationAidl"

#include "GnssPowerIndication.h"
#include <android/binder_auto_utils.h>
#include <log_util.h>
#include <inttypes.h>
#include "loc_misc_utils.h"

typedef const GnssInterface* (getLocationInterface)();

namespace android {
namespace hardware {
namespace gnss {
namespace aidl {
namespace implementation {

static GnssPowerIndication* spGnssPowerIndication = nullptr;

GnssPowerIndication::GnssPowerIndication() :
    mDeathRecipient(AIBinder_DeathRecipient_new(GnssPowerIndication::gnssPowerIndicationDied)) {
    spGnssPowerIndication = this;
}

GnssPowerIndication::~GnssPowerIndication() {
    spGnssPowerIndication = nullptr;
}

::ndk::ScopedAStatus GnssPowerIndication::setCallback(
        const std::shared_ptr<IGnssPowerIndicationCallback>& in_callback) {
    std::unique_lock<std::mutex> lock(mMutex);

    if (nullptr == in_callback) {
        LOC_LOGe("callback is nullptr");
        return ndk::ScopedAStatus::fromExceptionCode(STATUS_INVALID_OPERATION);
    }

    AIBinder_linkToDeath(in_callback->asBinder().get(), mDeathRecipient, this);
    mGnssPowerIndicationCb = in_callback;

    static bool getGnssInterfaceFailed = false;
    if (nullptr == mGnssInterface && !getGnssInterfaceFailed) {
        void * libHandle = nullptr;
        getLocationInterface* getter = (getLocationInterface*)
            dlGetSymFromLib(libHandle, "libgnss.so", "getGnssInterface");
        if (NULL == getter) {
            getGnssInterfaceFailed = true;
        } else {
            mGnssInterface = (GnssInterface*)(*getter)();
        }
    }
    if (nullptr != mGnssInterface) {
        mGnssInterface->powerIndicationInit(piGnssPowerIndicationCb);
    } else {
        LOC_LOGe("mGnssInterface is nullptr");
        return ndk::ScopedAStatus::fromExceptionCode(STATUS_INVALID_OPERATION);
    }

    lock.unlock();
    mGnssPowerIndicationCb->setCapabilitiesCb(IGnssPowerIndicationCallback::CAPABILITY_TOTAL);
    return ndk::ScopedAStatus::ok();
}

void GnssPowerIndication::cleanup() {
    LOC_LOGd("()");
    if (nullptr != mGnssPowerIndicationCb) {
        AIBinder_unlinkToDeath(mGnssPowerIndicationCb->asBinder().get(), mDeathRecipient, this);
        mGnssPowerIndicationCb = nullptr;
    }
}

void GnssPowerIndication::gnssPowerIndicationDied(void* cookie) {
    LOC_LOGe("IGnssPowerIndicationCallback service died");
    GnssPowerIndication* iface = static_cast<GnssPowerIndication*>(cookie);
    if (iface != nullptr) {
        iface->cleanup();
    }
}

ndk::ScopedAStatus GnssPowerIndication::requestGnssPowerStats() {
    LOC_LOGd("requestGnssPowerStats");
    std::unique_lock<std::mutex> lock(mMutex);

    if (nullptr != mGnssInterface) {
        lock.unlock();
        mGnssInterface->powerIndicationRequest();
    } else {
        LOC_LOGe("mGnssInterface is nullptr");
    }

    return ndk::ScopedAStatus::ok();
}

void GnssPowerIndication::piGnssPowerIndicationCb(GnssPowerStatistics gnssPowerStatistics) {
    if (nullptr != spGnssPowerIndication) {
        spGnssPowerIndication->gnssPowerIndicationCb(gnssPowerStatistics);
    } else {
        LOC_LOGe("spGnssPowerIndication is nullptr");
    }
}

void GnssPowerIndication::gnssPowerIndicationCb(GnssPowerStatistics gnssPowerStatistics) {

    GnssPowerStats gnssPowerStats = {};

    gnssPowerStats.elapsedRealtime.flags |= gnssPowerStats.elapsedRealtime.HAS_TIMESTAMP_NS;
    gnssPowerStats.elapsedRealtime.timestampNs = gnssPowerStatistics.elapsedRealTime;
    gnssPowerStats.elapsedRealtime.flags |= gnssPowerStats.elapsedRealtime.HAS_TIME_UNCERTAINTY_NS;
    gnssPowerStats.elapsedRealtime.timeUncertaintyNs = gnssPowerStatistics.elapsedRealTimeUnc;
    gnssPowerStats.totalEnergyMilliJoule = gnssPowerStatistics.totalEnergyMilliJoule;

    LOC_LOGd("gnssPowerStats.elapsedRealtime.flags: 0x%08X"
             " gnssPowerStats.elapsedRealtime.timestampNs: %" PRId64", "
             " gnssPowerStats.elapsedRealtime.timeUncertaintyNs: %.2f,"
             " gnssPowerStatistics.totalEnergyMilliJoule = %.2f",
             gnssPowerStats.elapsedRealtime.flags,
             gnssPowerStats.elapsedRealtime.timestampNs,
             gnssPowerStats.elapsedRealtime.timeUncertaintyNs,
             gnssPowerStats.totalEnergyMilliJoule);

    std::unique_lock<std::mutex> lock(mMutex);
    auto gnssPowerIndicationCb = mGnssPowerIndicationCb;
    lock.unlock();
    if (nullptr != gnssPowerIndicationCb) {
        gnssPowerIndicationCb->gnssPowerStatsCb(gnssPowerStats);
    }
}

}  // namespace implementation
}  // namespace aidl
}  // namespace gnss
}  // namespace hardware
}  // namespace android
