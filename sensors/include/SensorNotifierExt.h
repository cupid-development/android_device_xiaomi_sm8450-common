/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <android/frameworks/sensorservice/1.0/ISensorManager.h>

#include "SscCalApi.h"
#include "SensorNotifier.h"

using android::sp;
using android::frameworks::sensorservice::V1_0::ISensorManager;

class SensorNotifierExt {
  public:
    SensorNotifierExt(sp<ISensorManager> manager, process_msg_t processMsg);
    std::vector<std::unique_ptr<SensorNotifier>> mNotifiers;
};
