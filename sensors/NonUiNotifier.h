/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "SensorNotifier.h"

class NonUiNotifier : public SensorNotifier {
  public:
    NonUiNotifier(sp<ISensorManager> manager, process_msg_t processMsg);
    ~NonUiNotifier();

  protected:
    void pollingFunction();
};
