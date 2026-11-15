/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "Compensation.h"

namespace udals {
struct SensorSample {
    int64_t timestampNs;
    Channels channels;
};
struct DisplayState {
    int64_t timestampNs;
    int dbv;
    bool dc, active;
};
struct AlignedSample {
    SensorSample sensor;
    DisplayState display;
};

class Timeline {
  public:
    void sensor(const SensorSample& sample) {
        if (sample.timestampNs <= mLastProcessed ||
            (!mSensors.empty() && sample.timestampNs <= mSensors.back().timestampNs))
            return;
        mSensors.push_back(sample);
        if (mSensors.size() > 64) mSensors.pop_front();
    }
    void display(const DisplayState& state) {
        if (!mDisplay.empty() && state.timestampNs < mDisplay.back().timestampNs) return;
        mDisplay.push_back(state);
        if (mDisplay.size() > 64) mDisplay.pop_front();
    }
    std::optional<AlignedSample> align(int64_t captureNs) {
        for (const auto& sample : mSensors) {
            if (sample.timestampNs <= mLastProcessed || sample.timestampNs < captureNs) continue;
            if (sample.timestampNs - captureNs > 1000000000LL) return std::nullopt;
            for (auto it = mDisplay.rbegin(); it != mDisplay.rend(); ++it) {
                // Stock gives a brightness update 20 ms to reach the sensor.
                if (it->timestampNs > sample.timestampNs - 20000000LL) continue;
                mLastProcessed = sample.timestampNs;
                if (!it->active || it->dbv <= 0) return std::nullopt;
                return AlignedSample{sample, *it};
            }
            return std::nullopt;
        }
        return std::nullopt;
    }
    void clear() {
        mSensors.clear();
        mDisplay.clear();
        // Do not replay an old sensor timestamp after a power transition.
    }

  private:
    std::deque<SensorSample> mSensors;
    std::deque<DisplayState> mDisplay;
    int64_t mLastProcessed = 0;
};
}  // namespace udals
