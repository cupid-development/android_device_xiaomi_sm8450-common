/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "LightNotifier"

#include "LightNotifier.h"

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-base/unique_fd.h>
#include <display/drm/mi_disp.h>
#include <poll.h>
#include <sys/ioctl.h>

#include "SensorNotifierUtils.h"
#include "SscCalApi.h"

static const std::string kDispFeatureDevice = "/dev/mi_display/disp_feature";

LightNotifier::LightNotifier(sp<ISensorManager> manager) : SensorNotifier(manager) {
    std::stringstream lightSensorsPrimary(
            android::base::GetProperty("ro.vendor.sensors.notifier.light_sensors.primary", ""));
    std::stringstream lightSensorsSecondary(
            android::base::GetProperty("ro.vendor.sensors.notifier.light_sensors.secondary", ""));

    std::string sensor;
    while (std::getline(lightSensorsPrimary, sensor, ',')) {
        mLightSensorsPrimary.push_back(std::stoi(sensor));
    }
    while (std::getline(lightSensorsSecondary, sensor, ',')) {
        mLightSensorsSecondary.push_back(std::stoi(sensor));
    }
}

LightNotifier::~LightNotifier() {
    deactivate();
}

void LightNotifier::notify() {
    if (mLightSensorsPrimary.empty() && mLightSensorsSecondary.empty()) {
        LOG(DEBUG) << "no light sensors to notify defined, skip light notifications";
        mActive = false;
        return;
    }

    android::base::unique_fd disp_fd_ =
            android::base::unique_fd(open(kDispFeatureDevice.c_str(), O_RDWR));
    if (disp_fd_.get() == -1) {
        LOG(ERROR) << "failed to open " << kDispFeatureDevice;
        mActive = false;
        return;
    }

    std::vector<disp_display_type> displays;
    if (!mLightSensorsPrimary.empty()) {
        displays.push_back(MI_DISP_PRIMARY);
    }
    if (!mLightSensorsSecondary.empty()) {
        displays.push_back(MI_DISP_SECONDARY);
    }

    const std::vector<disp_event_type> notifyEvents = {MI_DISP_EVENT_POWER, MI_DISP_EVENT_FPS,
                                                       MI_DISP_EVENT_51_BRIGHTNESS,
                                                       MI_DISP_EVENT_HBM, MI_DISP_EVENT_DC};

    // Register for events
    for (const disp_display_type& display : displays) {
        for (const disp_event_type& event : notifyEvents) {
            disp_event_req req;
            req.base.flag = 0;
            req.base.disp_id = display;
            req.type = event;
            ioctl(disp_fd_.get(), MI_DISP_IOCTL_REGISTER_EVENT, &req);
        }
    }

    struct pollfd dispEventPoll = {
            .fd = disp_fd_.get(),
            .events = POLLIN,
    };

    _oem_msg* msg = new _oem_msg;
    notify_t notifyType;
    float value;

    while (mActive) {
        int rc = poll(&dispEventPoll, 1, -1);
        if (rc < 0) {
            LOG(ERROR) << "failed to poll " << kDispFeatureDevice << ", err: " << rc;
            continue;
        }

        struct disp_event_resp* response = parseDispEvent(disp_fd_.get());
        if (response == nullptr) {
            continue;
        }

        std::vector<int>& sensorsToNotify = mLightSensorsPrimary;
        switch (response->base.disp_id) {
            case MI_DISP_PRIMARY:
                break;
            case MI_DISP_SECONDARY:
                sensorsToNotify = mLightSensorsSecondary;
                break;
            default:
                LOG(ERROR) << "got notified for unknown display: " << response->base.disp_id;
                continue;
        }

        switch (response->base.type) {
            case MI_DISP_EVENT_POWER:
                notifyType = POWER_STATE;
                value = response->data[0];
                break;
            case MI_DISP_EVENT_FPS:
                notifyType = DISPLAY_FREQUENCY;
                value = response->data[0];
                break;
            case MI_DISP_EVENT_51_BRIGHTNESS:
                notifyType = BRIGHTNESS;
                value = *(uint16_t*)response->data;
                break;
            case MI_DISP_EVENT_HBM:
                notifyType = BRIGHTNESS;
                value = response->data[0] ? -1 : -2;
                break;
            case MI_DISP_EVENT_DC:
                notifyType = DC_STATE;
                value = response->data[0];
                break;
            default:
                LOG(ERROR) << "got unknown event: " << response->base.type;
                continue;
        };

        for (const auto sensorId : sensorsToNotify) {
            msg->sensorType = sensorId;
            msg->notifyType = notifyType;
            msg->notifyTypeFloat = notifyType;
            msg->value = value;
            msg->unknown1 = 1;
            msg->unknown2 = 5;

            SscCalApiWrapper::getInstance().processMsg(msg);
        }
    }
}
