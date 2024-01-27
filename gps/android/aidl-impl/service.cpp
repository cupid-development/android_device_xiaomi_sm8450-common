/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
 * Not a Contribution
 */
/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include <aidl/android/hardware/gnss/IGnss.h>
#include <android/hardware/gnss/2.1/IGnss.h>
#include <hidl/LegacySupport.h>
#include "loc_cfg.h"
#include "loc_misc_utils.h"
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include "Gnss.h"
#include <pthread.h>
#include <log_util.h>

extern "C" {
#include "vndfwk-detect.h"
}
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "android.hardware.gnss-aidl-impl-qti"

#ifdef ARCH_ARM_32
#define DEFAULT_HW_BINDER_MEM_SIZE 65536
#endif

using android::hardware::configureRpcThreadpool;
using android::hardware::registerPassthroughServiceImplementation;
using android::hardware::joinRpcThreadpool;
using ::android::sp;

using android::status_t;
using android::OK;

typedef int vendorEnhancedServiceMain(int /* argc */, char* /* argv */ []);

using GnssAidl = ::android::hardware::gnss::aidl::implementation::Gnss;
using ::android::hardware::gnss::V1_0::GnssLocation;
using android::hardware::gnss::V2_1::IGnss;

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    ALOGI("%s, start Gnss HAL process", __FUNCTION__);

    std::shared_ptr<GnssAidl> gnssAidl = ndk::SharedRefBase::make<GnssAidl>();
    const std::string instance = std::string() + GnssAidl::descriptor + "/default";
    if (gnssAidl != nullptr) {
        binder_status_t status =
            AServiceManager_addService(gnssAidl->asBinder().get(), instance.c_str());
        if (STATUS_OK == status) {
            ALOGD("register IGnss AIDL service success");
        } else {
            ALOGD("Error while register IGnss AIDL service, status: %d", status);
        }
    }

    int vendorInfo = getVendorEnhancedInfo();
    // The magic number 2 points to
    // #define VND_ENHANCED_SYS_STATUS_BIT 0x02 in vndfwk-detect.c
    bool vendorEnhanced = ( vendorInfo & 2 );
    setVendorEnhanced(vendorEnhanced);

#ifdef ARCH_ARM_32
    android::hardware::ProcessState::initWithMmapSize((size_t)(DEFAULT_HW_BINDER_MEM_SIZE));
#endif
    configureRpcThreadpool(1, true);

    status_t ret;
    ret = registerPassthroughServiceImplementation<IGnss>();
    if (ret == OK) {
        // Loc AIDL service
#define VENDOR_AIDL_LIB "vendor.qti.gnss-service.so"

        void* libAidlHandle = NULL;
        vendorEnhancedServiceMain* aidlMainMethod = (vendorEnhancedServiceMain*)
            dlGetSymFromLib(libAidlHandle, VENDOR_AIDL_LIB, "main");
        if (NULL != aidlMainMethod) {
            ALOGI("start LocAidl service");
            (*aidlMainMethod)(0, NULL);
        }
        // Loc AIDL service end
        joinRpcThreadpool();
        ABinderProcess_joinThreadPool();
    } else {
        ALOGE("Error while registering IGnss HIDL 2.1 service: %d", ret);
    }

    return EXIT_FAILURE;  // should not reach
}
