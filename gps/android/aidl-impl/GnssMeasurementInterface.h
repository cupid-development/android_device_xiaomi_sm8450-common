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

#include <aidl/android/hardware/gnss/BnGnssMeasurementInterface.h>
#include <aidl/android/hardware/gnss/BnGnssMeasurementCallback.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <LocationAPIClientBase.h>
#include <gps_extended_c.h>

using aidl::android::hardware::gnss::ElapsedRealtime;
using aidl::android::hardware::gnss::GnssClock;
using aidl::android::hardware::gnss::GnssData;
using aidl::android::hardware::gnss::GnssMeasurement;
using aidl::android::hardware::gnss::GnssSignalType;
using aidl::android::hardware::gnss::GnssConstellationType;

namespace android {
namespace hardware {
namespace gnss {
namespace aidl {
namespace implementation {

using ::aidl::android::hardware::gnss::BnGnssMeasurementInterface;
using ::aidl::android::hardware::gnss::IGnssMeasurementCallback;

struct GnssMeasurementInterface : public BnGnssMeasurementInterface, public LocationAPIClientBase {
public:
    GnssMeasurementInterface();
    ~GnssMeasurementInterface() {}
    ::ndk::ScopedAStatus setCallback(
            const std::shared_ptr<IGnssMeasurementCallback>& in_callback,
            bool in_enableFullTracking, bool in_enableCorrVecOutputs) override;
    ::ndk::ScopedAStatus close() override;

    // callbacks we are interested in
    void onGnssMeasurementsCb(GnssMeasurementsNotification gnssMeasurementsNotification) final;

private:
    std::shared_ptr<IGnssMeasurementCallback> mGnssMeasurementCbIface = nullptr;
    // Synchronization lock for mGnssMeasurementCbIface
    mutable std::mutex mMutex;
    AIBinder_DeathRecipient* mDeathRecipient;
    bool mTracking;

    static void gnssMeasurementDied(void* cookie);
    void startTracking(GnssPowerMode powerMode = GNSS_POWER_MODE_INVALID,
                       uint32_t timeBetweenMeasurement = GPS_DEFAULT_FIX_INTERVAL_MS);
    void convertGnssData(GnssMeasurementsNotification& in, GnssData& out);
    void convertGnssMeasurement(GnssMeasurementsData& in, GnssMeasurement& out);
    void convertGnssFlags(GnssMeasurementsData& in, GnssMeasurement& out);
    static void convertGnssSvId(GnssMeasurementsData& in, int& out);
    void convertGnssSignalType(GnssMeasurementsData& in, GnssSignalType& out);
    static void convertGnssConstellationType(GnssSvType& in, GnssConstellationType& out);
    void convertGnssMeasurementsCodeType(GnssMeasurementsCodeType& inCodeType,
                                         char* inOtherCodeTypeName, GnssSignalType& out);
    void convertGnssState(GnssMeasurementsData& in, GnssMeasurement& out);
    void convertGnssAccumulatedDeltaRangeState(GnssMeasurementsData& in, GnssMeasurement& out);
    void convertGnssMultipathIndicator(GnssMeasurementsData& in, GnssMeasurement& out);
    void convertGnssSatellitePvtFlags(GnssMeasurementsData& in, GnssMeasurement& out);
    void convertGnssSatellitePvt(GnssMeasurementsData& in, GnssMeasurement& out);
    void convertGnssClock(GnssMeasurementsClock& in, GnssClock& out);
    void convertElapsedRealtimeNanos(GnssMeasurementsNotification& in,
                                     ElapsedRealtime& elapsedRealtime);
    void printGnssData(GnssData& data);
};

}  // namespace implementation
}  // namespace aidl
}  // namespace gnss
}  // namespace hardware
}  // namespace android
