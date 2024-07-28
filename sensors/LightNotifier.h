/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "SensorNotifier.h"

class LightNotifier : public SensorNotifier {
  public:
    LightNotifier(sp<ISensorManager> manager, process_msg_t processMsg);
    ~LightNotifier();

  protected:
    void pollingFunction();

  private:
    std::vector<int> mLightSensors;
};
