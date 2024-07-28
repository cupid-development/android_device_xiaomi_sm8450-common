/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "SensorNotifier"

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <dlfcn.h>

#include "AodNotifier.h"
#include "LightNotifier.h"
#include "NonUiNotifier.h"

int main() {
    sp<ISensorManager> manager = ISensorManager::getService();
    if (manager == nullptr) {
        LOG(ERROR) << "failed to get ISensorManager";
        return EXIT_FAILURE;
    }

    process_msg_t process_msg;
    void* ssccalapiHandle = dlopen("libssccalapi@2.0.so", RTLD_NOW);
    if (ssccalapiHandle) {
        init_current_sensors_t init_current_sensors =
                (init_current_sensors_t)dlsym(ssccalapiHandle, "_Z20init_current_sensorsb");
        if (init_current_sensors != NULL) {
            init_current_sensors(
                    android::base::GetBoolProperty("persist.vendor.debug.ssccalapi", false));
        } else {
            LOG(ERROR) << "could not find init_current_sensors";
        }

        process_msg = (process_msg_t)dlsym(ssccalapiHandle, "_Z11process_msgP8_oem_msg");
        if (process_msg == NULL) {
            LOG(ERROR) << "could not find process_msg";
        }
    } else {
        LOG(INFO) << "could not dlopen libssccalapi@2.0.so";
    }

    std::unique_ptr<AodNotifier> aodNotifier = std::make_unique<AodNotifier>(manager, process_msg);
    aodNotifier->activate();

    std::unique_ptr<LightNotifier> lightNotifier;
    if (process_msg != NULL &&
        android::base::GetProperty("ro.vendor.sensors.notifier.light_sensors", "") != "") {
        lightNotifier = std::make_unique<LightNotifier>(manager, process_msg);
        lightNotifier->activate();
    }

    std::unique_ptr<NonUiNotifier> nonUiNotifier =
            std::make_unique<NonUiNotifier>(manager, process_msg);
    nonUiNotifier->activate();

    while (true) {
        // Sleep to keep the notifiers alive
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    // Should never reach this
    return EXIT_SUCCESS;
}
