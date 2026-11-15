/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */
#define LOG_TAG "UdalsNotifier"

#include "UdalsNotifier.h"
#include "Capture.h"
#include "SscCalApi.h"
#include "Timeline.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/parseint.h>
#include <android-base/strings.h>
#include <android/binder_process.h>
#include <time.h>
#include <cmath>
#include <mutex>

namespace udals {
using android::hardware::Return;
using android::hardware::Void;
using android::hardware::sensors::V1_0::Event;
namespace {
constexpr uint32_t kRawLight = 33171111;
constexpr auto kDisplay = "/sys/devices/platform/soc/soc:qcom,dsi-display-primary/";
int64_t nowNs() {
    timespec time{};
    clock_gettime(CLOCK_BOOTTIME, &time);
    return time.tv_sec * 1000000000LL + time.tv_nsec;
}
std::optional<int> readState(const char* name) {
    std::string value;
    int number;
    if (!android::base::ReadFileToString(std::string(kDisplay) + name, &value) ||
        !android::base::ParseInt(android::base::Trim(value), &number) || number < 0)
        return std::nullopt;
    return number;
}
}  // namespace

struct UdalsNotifier::Events : public IEventQueueCallback {
    std::mutex lock;
    std::deque<SensorSample> samples;
    Return<void> onEvent(const Event& event) override {
        if (static_cast<uint32_t>(event.sensorType) != kRawLight || event.timestamp <= 0)
            return Void();
        SensorSample sample{event.timestamp,
                            {event.u.vec4.x, event.u.vec4.y, event.u.vec4.z, event.u.vec4.w}};
        for (double value : sample.channels)
            if (!std::isfinite(value) || value < 0) return Void();
        std::lock_guard guard(lock);
        if (samples.size() == 64) samples.pop_front();
        samples.push_back(sample);
        return Void();
    }
};

UdalsNotifier::UdalsNotifier(sp<ISensorManager> manager) : SensorNotifier(manager) {
    std::string error;
    if (!mCompensation.load("/vendor/etc/sensors/udals-model.json",
                            "/vendor/etc/sensors/config/lightSensorConfig.json",
                            "/mnt/vendor/persist/sensors/lightSensorCali.json", &error)) {
        LOG(ERROR) << "UDALS configuration rejected: " << error;
        return;
    }
    // HIDL callbacks retain an independent queue, never a pointer to this object.
    mEvents = new Events();
    mReady = initializeSensorQueue("xiaomi.sensor.ambientlight.raw", false, mEvents) == Result::OK;
}

UdalsNotifier::~UdalsNotifier() {
    deactivate();
}

void UdalsNotifier::notify() {
    if (!mReady) return;
    ABinderProcess_startThreadPool();
    Capture capture(mCompensation);
    Timeline timeline;
    std::optional<CaptureResult> frame;
    DisplayState previous{};
    bool enabled = false;
    int64_t screenOn = 0;
    int64_t lastWarning = 0;
    while (mActive) {
        const int64_t now = nowNs();
        auto power = readState("panel_power_state");
        auto dbv = readState("dbv");
        // The current driver does not expose DC dimming. Missing state means
        // normal PWM; a user preference property is not proof of applied DC.
        const bool dc = readState("dc_mode").value_or(0) != 0;
        DisplayState state{now, dbv.value_or(0), dc, power && *power == 0 && dbv.has_value()};
        const bool changed = state.active != previous.active || state.dbv != previous.dbv ||
                             state.dc != previous.dc;
        if (state.active != enabled) {
            auto status = state.active ? mQueue->enableSensor(mSensorHandle, 50000, 0)
                                       : mQueue->disableSensor(mSensorHandle);
            if (!status.isOk() || status != Result::OK) {
                if (now - lastWarning > 5000000000LL) {
                    LOG(ERROR) << "Cannot change UDALS sensor activation";
                    lastWarning = now;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            enabled = state.active;
            screenOn = now;
            frame.reset();
            timeline.clear();
            mCompensation.reset();
            std::lock_guard guard(mEvents->lock);
            mEvents->samples.clear();
        }
        if (changed || !previous.timestampNs) timeline.display(state);
        previous = state;
        {
            std::lock_guard guard(mEvents->lock);
            for (const auto& sample : mEvents->samples)
                if (sample.timestampNs >= screenOn) timeline.sensor(sample);
            mEvents->samples.clear();
        }
        if (auto completed = capture.take();
            completed && completed->submittedNs >= screenOn && enabled) {
            frame = completed;
        }
        if (enabled && state.dbv > 0) {
            if (frame && now - frame->completedNs > 1000000000LL) frame.reset();
            if (frame && now - screenOn >= 300000000LL) {
                if (auto aligned = timeline.align(frame->completedNs)) {
                    const int brightness = std::min(aligned->display.dbv, 2047);
                    auto bleed = mCompensation.predict(brightness, aligned->display.dc, frame->rgb);
                    auto lux = mCompensation.filter(
                            mCompensation.lux(aligned->sensor.channels, bleed, brightness));
                    if (lux) {
                        _oem_msg message{};
                        message.sensorType = kRawLight;
                        message.notifyType = REPORT_VALUE;
                        message.notifyTypeFloat = REPORT_VALUE;
                        message.unknown1 = mCompensation.isMarble() ? 2 : 1;
                        message.unknown2 = 5;
                        message.value = *lux;
                        if (SscCalApiWrapper::getInstance().processMsg(&message) == 0)
                            mCompensation.reported(*lux);
                    }
                    frame.reset();
                }
            }
            if (!frame) capture.request(now);
        }
        // Bounded polling also observes applied brightness while the panel is
        // idle, without depending on optional sysfs notification support.
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    if (enabled) mQueue->disableSensor(mSensorHandle);
}
}  // namespace udals
