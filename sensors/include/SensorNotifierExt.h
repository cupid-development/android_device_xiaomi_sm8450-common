/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <android/frameworks/sensorservice/1.0/ISensorManager.h>

using android::frameworks::sensorservice::V1_0::ISensorManager;

void activateAdditionalNotifiers(sp<ISensorManager> manager);
