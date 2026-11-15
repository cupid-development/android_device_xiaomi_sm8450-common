/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */
#include "SensorNotifierExt.h"
#include "UdalsNotifier.h"

SensorNotifierExt::SensorNotifierExt(sp<ISensorManager> manager) {
    mNotifiers.push_back(std::make_unique<udals::UdalsNotifier>(manager));
}
