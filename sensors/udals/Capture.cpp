/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */
#include "Capture.h"

#include <aidl/vendor/qti/hardware/display/config/BnDisplayConfigCallback.h>
#include <aidlcommonsupport/NativeHandle.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/hardware_buffer.h>
#include <time.h>
#include <vndk/hardware_buffer.h>
#include <mutex>

namespace udals {
namespace config = aidl::vendor::qti::hardware::display::config;
using aidl::android::hardware::common::NativeHandle;

struct Capture::Request : config::BnDisplayConfigCallback {
    AHardwareBuffer* buffer = nullptr;
    int64_t submittedNs = 0, completedNs = 0;
    int error = 0;
    bool done = false;
    std::mutex lock;

    ~Request() override {
        if (buffer) AHardwareBuffer_release(buffer);
    }
    ndk::ScopedAStatus notifyCWBBufferDone(int32_t status, const NativeHandle&) override {
        std::lock_guard guard(lock);
        timespec now{};
        clock_gettime(CLOCK_BOOTTIME, &now);
        completedNs = now.tv_sec * 1000000000LL + now.tv_nsec;
        error = status;
        done = true;
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus notifyQsyncChange(bool, int32_t, int32_t) override {
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus notifyIdleStatus(bool) override { return ndk::ScopedAStatus::ok(); }
    ndk::ScopedAStatus notifyCameraSmoothInfo(config::CameraSmoothOp, int32_t) override {
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus notifyResolutionChange(int32_t, const config::Attributes&) override {
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus notifyFpsMitigation(int32_t, const config::Attributes&,
                                           config::Concurrency) override {
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus notifyTUIEventDone(int32_t, config::DisplayType,
                                          config::TUIEventType) override {
        return ndk::ScopedAStatus::ok();
    }
};

Capture::~Capture() {
    if (mBuffer) AHardwareBuffer_release(mBuffer);
}
bool Capture::failed(const std::string& reason) {
    if (++mFailures == 1 || mFailures % 30 == 0) {
        LOG(WARNING) << "UDALS capture unavailable: " << reason << " (" << mFailures
                     << " failures)";
    }
    return false;
}

bool Capture::request(int64_t now) {
    bool alive = true;
    if (mDisplay && now - mLastPingNs >= 1000000000LL) {
        mLastPingNs = now;
        alive = AIBinder_ping(mDisplay->asBinder().get()) == STATUS_OK;
    }
    if (!alive) {
        mDisplay.reset();
        mRequest.reset();
        if (mBuffer) AHardwareBuffer_release(mBuffer);
        mBuffer = nullptr;
    }
    if (mRequest || now < mRetryAfterNs) return false;
    mRetryAfterNs = now + 1000000000LL;
    if (!mDisplay) {
        const std::string name = std::string(config::IDisplayConfig::descriptor) + "/default";
        mDisplay = config::IDisplayConfig::fromBinder(
                ndk::SpAIBinder(AServiceManager_checkService(name.c_str())));
        if (!mDisplay) return failed("display config service unavailable");
    }
    config::Attributes attributes;
    if (!mDisplay->getActiveBuiltinDisplayAttributes(&attributes).isOk()) {
        return failed("cannot query display dimensions");
    }
    const auto region = mCompensation.region();
    if (attributes.xRes < region.left + region.width ||
        attributes.yRes < region.top + region.height || attributes.xRes > 8192 ||
        attributes.yRes > 8192)
        return failed("sensor region outside display dimensions");

    auto request = ndk::SharedRefBase::make<Request>();
    // AIDL V7 does not return the adjusted crop or required buffer dimensions.
    // Full-frame readback avoids interpreting an expanded/fallback crop as the
    // sensor ROI. Only the sensor's small region is read on the CPU.
    AHardwareBuffer_Desc description{};
    description.width = attributes.xRes;
    description.height = attributes.yRes;
    description.layers = 1;
    description.format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;
    description.usage = AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN |
                        AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT |
                        AHARDWAREBUFFER_USAGE_COMPOSER_OVERLAY;
    if (mBuffer) {
        AHardwareBuffer_Desc allocated{};
        AHardwareBuffer_describe(mBuffer, &allocated);
        if (allocated.width != description.width || allocated.height != description.height) {
            AHardwareBuffer_release(mBuffer);
            mBuffer = nullptr;
        }
    }
    if (!mBuffer && AHardwareBuffer_allocate(&description, &mBuffer))
        return failed("buffer allocation");
    request->buffer = mBuffer;
    AHardwareBuffer_acquire(request->buffer);
    const auto* handle = AHardwareBuffer_getNativeHandle(request->buffer);
    request->submittedNs = now;
    config::Rect rect{0, 0, attributes.xRes, attributes.yRes};
    auto status = mDisplay->setCWBOutputBuffer(request,
                                               static_cast<int32_t>(config::DisplayType::PRIMARY),
                                               rect, true, android::dupToAidl(handle));
    if (!status.isOk()) {
        if (status.getStatus() == STATUS_DEAD_OBJECT) mDisplay.reset();
        // A failed transaction is not a cancellation acknowledgment. Retire the
        // allocation; the server owns its imported handle if it accepted it.
        AHardwareBuffer_release(mBuffer);
        mBuffer = nullptr;
        return failed(status.getDescription());
    }
    mRequest = std::move(request);
    mRetryAfterNs = now + 200000000LL;
    // Ensure a static scene produces a frame for this request too.
    mDisplay->refreshScreen();
    return true;
}

std::optional<CaptureResult> Capture::take() {
    if (!mRequest) return std::nullopt;
    auto request = mRequest;
    std::lock_guard guard(request->lock);
    if (!request->done) return std::nullopt;
    mRequest.reset();
    if (request->error) {
        failed("completion status " + std::to_string(request->error));
        return std::nullopt;
    }
    AHardwareBuffer_Desc description{};
    AHardwareBuffer_describe(request->buffer, &description);
    void* pixels = nullptr;
    if (description.stride < description.width ||
        AHardwareBuffer_lock(request->buffer, AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, -1, nullptr,
                             &pixels)) {
        failed("CPU buffer mapping");
        return std::nullopt;
    }
    CaptureResult result{
            request->submittedNs, request->completedNs,
            mCompensation.sample(static_cast<uint8_t*>(pixels), description.stride, 0, 0)};
    AHardwareBuffer_unlock(request->buffer, nullptr);
    mFailures = 0;
    return result;
}
}  // namespace udals
