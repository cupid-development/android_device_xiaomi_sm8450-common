/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "Compensation.h"

#include <aidl/vendor/qti/hardware/display/config/IDisplayConfig.h>
#include <memory>
#include <optional>

struct AHardwareBuffer;

namespace udals {
struct CaptureResult {
    int64_t submittedNs, completedNs;
    Rgb rgb;
};
class Capture {
  public:
    explicit Capture(const Compensation& compensation) : mCompensation(compensation) {}
    ~Capture();
    // Nonblocking. A timeout does not cancel a request or make its buffer reusable.
    bool request(int64_t now);
    std::optional<CaptureResult> take();

  private:
    struct Request;
    bool failed(const std::string& reason);
    const Compensation& mCompensation;
    std::shared_ptr<aidl::vendor::qti::hardware::display::config::IDisplayConfig> mDisplay;
    std::shared_ptr<Request> mRequest;
    AHardwareBuffer* mBuffer = nullptr;
    int64_t mRetryAfterNs = 0;
    int64_t mLastPingNs = 0;
    unsigned mFailures = 0;
};
}  // namespace udals
