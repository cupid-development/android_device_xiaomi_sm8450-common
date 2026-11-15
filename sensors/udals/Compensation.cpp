/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */
#include "Compensation.h"

#include <json/json.h>

#include <sys/stat.h>
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <fstream>
#include <numeric>
#include <sstream>

namespace udals {
namespace {
bool readJson(const std::string& path, Json::Value* value, std::string* error) {
    std::ifstream input(path);
    if (!input) {
        *error = "Cannot read " + path;
        return false;
    }
    Json::CharReaderBuilder builder;
    builder["rejectDupKeys"] = true;
    return Json::parseFromStream(builder, input, value, error);
}

bool numbers(const Json::Value& value, size_t count, std::vector<double>* output) {
    std::vector<double> result;
    if (value.isString()) {
        std::istringstream stream(value.asString());
        std::string column;
        while (std::getline(stream, column, ',')) {
            std::istringstream number(column);
            double v;
            if (!(number >> v) || !std::isfinite(v)) return false;
            number >> std::ws;
            if (!number.eof()) return false;
            result.push_back(v);
        }
    } else if (value.isArray()) {
        for (const auto& column : value) {
            if (!column.isNumeric() || !std::isfinite(column.asDouble())) return false;
            result.push_back(column.asDouble());
        }
    } else {
        return false;
    }
    if (result.size() != count) return false;
    *output = std::move(result);
    return true;
}

bool channels(const Json::Value& value, Channels* output) {
    std::vector<double> values;
    if (!numbers(value, 4, &values)) return false;
    std::copy(values.begin(), values.end(), output->begin());
    return true;
}
}  // namespace

bool Compensation::load(const std::string& modelPath, const std::string& configPath,
                        const std::string& calibrationPath, std::string* error) {
    // Commit a complete validated configuration; a failed reload cannot mix profiles.
    Compensation parsed;
    Json::Value model, config, calibration;
    if (!readJson(modelPath, &model, error) || !readJson(configPath, &config, error)) return false;
    *error = "Invalid UDALS model or calibration";
    if (model["version"] != 1 || !model["device"].isString()) return false;
    const std::string device = model["device"].asString();
    if (device != "marble" && device != "mayfly") return false;
    parsed.mMarble = device == "marble";
    parsed.mGrid = parsed.mMarble ? 19 : 25;
    if (model["grid"] != parsed.mGrid || model["block_size"] != 5) return false;
    if (!model["gamma"].isNumeric()) return false;
    double gamma = model["gamma"].asDouble();
    if (!std::isfinite(gamma) || gamma < 1 || gamma > 4) return false;
    for (size_t i = 0; i < parsed.mGamma.size(); ++i) parsed.mGamma[i] = std::pow(i, gamma);
    if (!model["spatial"].isArray() || model["spatial"].size() != 3) return false;
    for (unsigned i = 0; i < 3; ++i) {
        if (!numbers(model["spatial"][i], parsed.mGrid * parsed.mGrid, &parsed.mSpatial[i]))
            return false;
    }
    const auto& als = config["Als"];
    const std::string owner = parsed.mMarble ? "sns_tcs3701" : "sns_tmd3719";
    if (als["panel_Info_cali_ori"]["owner"] != owner || als["channel_coef"]["owner"] != owner)
        return false;
    std::vector<double> center, radii;
    if (!numbers(als["cwbInfo"]["Center"], 2, &center) ||
        !numbers(als["cwbInfo"]["Radius"], 3, &radii))
        return false;
    const int width = parsed.mGrid * 5;
    if (radii[2] * 2 + 1 != width || radii[0] <= 0 || radii[1] <= radii[0] || radii[2] <= radii[1])
        return false;
    if (center[0] != std::floor(center[0]) || center[1] != std::floor(center[1]) ||
        center[0] < radii[2] || center[1] < radii[2] || center[0] > 8192 || center[1] > 8192)
        return false;
    parsed.mRegion = {static_cast<int>(center[0] - radii[2]),
                      static_cast<int>(center[1] - radii[2]), width, width};
    if (!numbers(als["channel_coef"]["coef_data"], parsed.mMarble ? 5 : 9, &parsed.mCoefficients))
        return false;
    auto curve = [](const Json::Value& value, Curve* output) {
        if (!value.isArray() || value.size() < 2 || value.size() > 256) return false;
        Curve result;
        for (const auto& row : value) {
            std::vector<double> columns;
            if (!numbers(row, 5, &columns) || columns[0] < 0 || columns[0] > 4095 ||
                columns[0] != std::floor(columns[0]) ||
                (!result.empty() && columns[0] <= result.back().dbv))
                return false;
            Point point{static_cast<int>(columns[0]), {}};
            for (size_t i = 0; i < 4; ++i) {
                if (columns[i + 1] < 0) return false;
                point.channels[i] = columns[i + 1];
            }
            result.push_back(point);
        }
        *output = std::move(result);
        return true;
    };
    if (!curve(als["panel_Info_cali_ori"]["data_w"], &parsed.mOriginal)) return false;
    parsed.mOriginalDc = parsed.mOriginal;
    if (als.isMember("panel_Info_cali_ori_dc") &&
        !curve(als["panel_Info_cali_ori_dc"]["data_w"], &parsed.mOriginalDc))
        return false;
    // Missing per-unit calibration means unity correction, as in stock. A present
    // but malformed file is rejected instead of silently mixing calibrations.
    struct stat calibrationStat{};
    const bool hasCalibration =
            !calibrationPath.empty() && stat(calibrationPath.c_str(), &calibrationStat) == 0;
    if (!calibrationPath.empty() && !hasCalibration && errno != ENOENT) {
        *error = "Cannot access per-unit calibration: " + calibrationPath;
        return false;
    }
    if (hasCalibration) {
        if (!readJson(calibrationPath, &calibration, error)) return false;
        const auto& cal = calibration["Als"];
        if (cal.isMember("panel_Info_cali") &&
            !curve(cal["panel_Info_cali"]["data_w"], &parsed.mCalibrated))
            return false;
        if (cal.isMember("panel_Info_cali_dc") &&
            !curve(cal["panel_Info_cali_dc"]["data_w"], &parsed.mCalibratedDc))
            return false;
        if (cal.isMember("channel_scale") &&
            !channels(cal["channel_scale"]["scale_data"], &parsed.mScale))
            return false;
        for (double scale : parsed.mScale)
            if (scale <= 0) return false;
    }
    if (parsed.mMarble) {
        if (!channels(model["input_offset"], &parsed.mOffset) ||
            !channels(model["input_divisor"], &parsed.mDivisor))
            return false;
        for (double divisor : parsed.mDivisor)
            if (divisor <= 0) return false;
    } else {
        std::vector<double> thresholds;
        if (!numbers(model["report_thresholds"], 3, &thresholds)) return false;
        std::copy(thresholds.begin(), thresholds.end(), parsed.mThresholds.begin());
    }
    if (!model["networks"].isArray() || model["networks"].size() != 4) return false;
    for (const auto& network : model["networks"]) {
        Network net;
        const unsigned outputs = parsed.mMarble ? 1 : 4;
        if (!numbers(network["exponent"], outputs, &net.exponent) ||
            !numbers(network["divisor"], outputs, &net.divisor))
            return false;
        for (double divisor : net.divisor)
            if (divisor <= 0) return false;
        if (!network["negative"].isString()) return false;
        const auto negative = network["negative"].asString();
        if (negative == "allow")
            net.negative = Network::Negative::Allow;
        else if (negative == "zero")
            net.negative = Network::Negative::Zero;
        else if (negative == "clamp")
            net.negative = Network::Negative::Clamp;
        else
            return false;
        if (!network["layers"].isArray() || network["layers"].size() != 4) return false;
        unsigned inputs = 4;
        for (unsigned index = 0; index < 4; ++index) {
            const auto& layer = network["layers"][index];
            if (!layer["outputs"].isUInt() || !layer["inputs"].isUInt() ||
                layer["inputs"].asUInt() != inputs)
                return false;
            unsigned width = layer["outputs"].asUInt();
            if ((index == 0 && width != 128 && width != 256) || (index == 1 && width != 64) ||
                (index == 2 && width != 32) || (index == 3 && width != outputs))
                return false;
            Layer dense{inputs, width, {}, {}};
            if (!numbers(layer["weights"], inputs * width, &dense.weights) ||
                !numbers(layer["bias"], width, &dense.bias))
                return false;
            net.layers.push_back(std::move(dense));
            inputs = width;
        }
        parsed.mNetworks.push_back(std::move(net));
    }
    *this = std::move(parsed);
    error->clear();
    return true;
}

std::vector<double> Compensation::Network::evaluate(const Channels& input) const {
    std::vector<double> values(input.begin(), input.end());
    for (size_t l = 0; l < layers.size(); ++l) {
        const auto& layer = layers[l];
        std::vector<double> next(layer.outputs);
        for (size_t j = 0; j < layer.outputs; ++j) {
            double sum = 0;
            for (size_t i = 0; i < layer.inputs; ++i)
                sum += values[i] * layer.weights[i * layer.outputs + j];
            sum += layer.bias[j];
            next[j] = l + 1 == layers.size() ? sum : std::max(0.0, sum);
        }
        values = std::move(next);
    }
    for (size_t i = 0; i < values.size(); ++i) {
        const double output = (std::pow(10.0, exponent[i] - values[i] / 100.0) - 1) / divisor[i];
        values[i] = negative == Negative::Zero && values[i] < 0 ? 0 : output;
        if (negative == Negative::Clamp) values[i] = std::max(0.0, values[i]);
    }
    return values;
}

Rgb Compensation::sample(const uint8_t* rgba, size_t stridePixels, int left, int top) const {
    Rgb sum{};
    // Stock visits 5x5 blocks in row order, then pixels in column order. Keep the
    // accumulation order and unnormalized sums expected by its trained model.
    for (int by = 0; by < mGrid; ++by) {
        for (int bx = 0; bx < mGrid; ++bx) {
            const int block = by * mGrid + bx;
            for (int x = 0; x < 5; ++x) {
                for (int y = 0; y < 5; ++y) {
                    const auto* pixel =
                            rgba + 4 * ((mRegion.top - top + by * 5 + y) * stridePixels +
                                        mRegion.left - left + bx * 5 + x);
                    for (size_t c = 0; c < 3; ++c) sum[c] += mGamma[pixel[c]] * mSpatial[c][block];
                }
            }
        }
    }
    return sum;
}

Channels Compensation::interpolate(const Curve& curve, int dbv) {
    if (dbv <= curve.front().dbv) return curve.front().channels;
    if (dbv >= curve.back().dbv) return curve.back().channels;
    auto upper = std::upper_bound(curve.begin(), curve.end(), dbv,
                                  [](int value, const Point& point) { return value < point.dbv; });
    const auto& lower = *(upper - 1);
    Channels result{};
    for (size_t i = 0; i < 4; ++i)
        result[i] = lower.channels[i] + (upper->channels[i] - lower.channels[i]) *
                                                (dbv - lower.dbv) / (upper->dbv - lower.dbv);
    return result;
}

Channels Compensation::predict(int dbv, bool dc, const Rgb& rgb) const {
    Channels result{};
    if (dbv <= 0) return result;
    dbv = std::min(dbv, 2047);
    Channels input{static_cast<double>(dbv), rgb[0], rgb[1], rgb[2]};
    if (mMarble) {
        for (size_t i = 0; i < 4; ++i) input[i] = (input[i] - mOffset[i]) / mDivisor[i];
        for (size_t i = 0; i < 4; ++i) result[i] = mNetworks[i].evaluate(input)[0];
    } else {
        for (double& value : input) value = std::log(value <= 1 ? value + 2 : value);
        const size_t index = dbv > 900 ? 3 : dbv > 400 ? 2 : dc ? 1 : 0;
        auto predicted = mNetworks[index].evaluate(input);
        std::copy(predicted.begin(), predicted.end(), result.begin());
    }
    const bool useDc = !mMarble && dc && dbv <= 400;
    const auto& original = useDc ? mOriginalDc : mOriginal;
    const auto& calibrated = useDc ? mCalibratedDc : mCalibrated;
    // Stock applies the ratio only when per-unit and reference tables match.
    if (calibrated.size() == original.size() && !calibrated.empty() &&
        dbv > calibrated.front().dbv && dbv <= calibrated.back().dbv) {
        const auto reference = interpolate(original, dbv);
        const auto measured = interpolate(calibrated, dbv);
        for (size_t i = 0; i < 4; ++i)
            if (reference[i] > 0) result[i] *= measured[i] / reference[i];
    }
    return result;
}

float Compensation::lux(const Channels& raw, const Channels& bleed, int dbv) const {
    Channels corrected{};
    for (size_t i = 0; i < 4; ++i) corrected[i] = std::max(0.0, raw[i] - bleed[i]) * mScale[i];
    if (mMarble && (raw[0] > 16775000 || raw[1] > 16775000 || raw[2] > 16775000)) return 200000;
    // The stock implementation rounds each accumulated channel term to float.
    float value = 0;
    for (size_t i = 0; i < 4; ++i)
        value = static_cast<float>(value + corrected[i] * mCoefficients[i + 1]);
    if (mMarble) {
        value = static_cast<float>(value * mCoefficients[0]) / 51200.0f;
    } else {
        if (dbv > 1700) value += 20;
        if (value < 30) value -= 2;
    }
    return std::max(0.0f, value);
}

std::optional<float> Compensation::filter(float value) {
    if (!std::isfinite(value)) return std::nullopt;
    if (!mMarble) {
        if (mLowLux.empty()) {
            mLowLux.push_back(value);
        } else {
            const size_t count = mLowLux.size();
            double mean = std::accumulate(mLowLux.begin(), mLowLux.end(), 0.0) / count;
            if ((mean < 8 && std::abs(value - mean) <= 4) ||
                (mean >= 8 && mean < 20 && std::abs(value - mean) <= 8)) {
                if (count == 7) mLowLux.pop_front();
                mLowLux.push_back(value);
                // Stock averages the previous count, excluding the new element
                // until its seven-element history is full.
                value = std::accumulate(mLowLux.begin(), mLowLux.begin() + count, 0.0) / count;
            } else {
                mLowLux.clear();
            }
        }
    }
    const size_t count = mMarble ? 3 : 5;
    mMedian.push_back(value);
    if (mMedian.size() > count) mMedian.pop_front();
    if (mMedian.size() == count) {
        auto sorted = mMedian;
        std::sort(sorted.begin(), sorted.end());
        value = mMarble ? sorted[count / 2] : static_cast<int>(sorted[count / 2]);
    }
    if (mLastReported) {
        const double previous = *mLastReported;
        const double threshold = mMarble         ? 0.05
                                 : previous < 10 ? mThresholds[0]
                                 : previous < 30 ? mThresholds[1]
                                 : previous < 60 ? 0.3
                                                 : mThresholds[2];
        const double difference = std::abs(value - previous);
        if (mMarble ? difference < previous * threshold : difference <= previous * threshold)
            return std::nullopt;
    }
    return value;
}

void Compensation::reset() {
    mLowLux.clear();
    mMedian.clear();
    mLastReported.reset();
}
}  // namespace udals
