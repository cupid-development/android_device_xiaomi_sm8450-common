/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "Compensation.h"
#include "SensorNotifier.h"

namespace udals {
class UdalsNotifier : public SensorNotifier {
  public:
    explicit UdalsNotifier(sp<ISensorManager> manager);
    ~UdalsNotifier() override;

  private:
    struct Events;
    void notify() override;
    Compensation mCompensation;
    sp<Events> mEvents;
    bool mReady = false;
};
}  // namespace udals
