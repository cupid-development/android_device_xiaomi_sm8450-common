/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <android-base/unique_fd.h>

#include "SensorNotifier.h"

class LightNotifier : public SensorNotifier {
  public:
    LightNotifier(sp<ISensorManager> manager);
    ~LightNotifier();

  protected:
    void notify();

  private:
    std::vector<int> mLightSensorsPrimary;
    std::vector<int> mLightSensorsSecondary;
};
