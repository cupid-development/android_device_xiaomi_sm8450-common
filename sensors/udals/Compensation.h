/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <vector>

namespace udals {

using Channels = std::array<double, 4>;  // Clear, red, green, blue, in sensor units.
using Rgb = std::array<double, 3>;

struct Rect {
    int left, top, width, height;
};

// Independent of Binder, gralloc and sensor transport; also built for host tests.
class Compensation {
  public:
    bool load(const std::string& model, const std::string& config, const std::string& calibration,
              std::string* error);
    Rect region() const { return mRegion; }
    bool isMarble() const { return mMarble; }
    Rgb sample(const uint8_t* rgba, size_t stridePixels, int left, int top) const;
    Channels predict(int dbv, bool dc, const Rgb& rgb) const;
    float lux(const Channels& raw, const Channels& bleed, int dbv) const;
    std::optional<float> filter(float lux);
    void reported(float lux) { mLastReported = lux; }
    void reset();

  private:
    struct Layer {
        size_t inputs, outputs;
        std::vector<double> weights, bias;
    };
    struct Network {
        std::vector<Layer> layers;
        std::vector<double> exponent, divisor;
        enum class Negative { Allow, Clamp, Zero } negative;
        std::vector<double> evaluate(const Channels& input) const;
    };
    struct Point {
        int dbv;
        Channels channels;
    };
    using Curve = std::vector<Point>;
    static Channels interpolate(const Curve& curve, int dbv);
    bool mMarble = false;
    Rect mRegion{};
    int mGrid = 0;
    std::array<std::vector<double>, 3> mSpatial;
    std::array<double, 256> mGamma{};
    Channels mOffset{}, mDivisor{1, 1, 1, 1}, mScale{1, 1, 1, 1};
    std::vector<double> mCoefficients;
    std::vector<Network> mNetworks;
    Curve mOriginal, mCalibrated, mOriginalDc, mCalibratedDc;
    std::array<double, 3> mThresholds{};
    std::deque<double> mLowLux, mMedian;
    std::optional<float> mLastReported;
};

}  // namespace udals
