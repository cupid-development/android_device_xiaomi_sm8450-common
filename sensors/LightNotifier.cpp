/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "LightNotifier"

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-base/unique_fd.h>
#include <display/drm/mi_disp.h>
#include <poll.h>
#include <sys/ioctl.h>

#include "LightNotifier.h"
#include "SensorNotifierUtils.h"

#define DISP_FEATURE_PATH "/dev/mi_display/disp_feature"

LightNotifier::LightNotifier(sp<ISensorManager> manager,
                             process_msg_t processMsg)
    : SensorNotifier(manager, processMsg) {
    mDispFd = android::base::unique_fd(open(DISP_FEATURE_PATH, O_RDWR));
    if (mDispFd.get() == -1) {
        LOG(ERROR) << "failed to open " << DISP_FEATURE_PATH;
        return;
    }

    // Check if a secondary panel is connected
    disp_panel_info req;
    req.base.flag = 0;
    req.base.disp_id = MI_DISP_SECONDARY;
    req.info_len = 0;
    req.info = nullptr;
    int res = ioctl(mDispFd.get(), MI_DISP_IOCTL_GET_PANEL_INFO, &req);
    mHasSecondaryPanel = res != -EINVAL;

    LOG(INFO) << "device has secondary panel: " << mHasSecondaryPanel;

    std::stringstream light_sensors_primary(
        mHasSecondaryPanel
            ? android::base::GetProperty(
                  "ro.vendor.sensors.notifier.light_sensors.primary", "")
            : android::base::GetProperty(
                  "ro.vendor.sensors.notifier.light_sensors", ""));
    std::stringstream light_sensors_secondary(android::base::GetProperty(
        "ro.vendor.sensors.notifier.light_sensors.secondary", ""));
    std::string sensor;

    while (std::getline(light_sensors_primary, sensor, ',')) {
        mLightSensorsPrimary.push_back(std::stoi(sensor));
    }

    while (std::getline(light_sensors_secondary, sensor, ',')) {
        mLightSensorsSecondary.push_back(std::stoi(sensor));
    }
}

LightNotifier::~LightNotifier() {
    LOG(ERROR) << "deconstructor";
    deactivate();
}

void LightNotifier::pollingFunction() {
    Result res;

    if (mProcessMsg == NULL) {
        LOG(ERROR) << "process_msg is unavailable, skip light notifications";
        mActive = false;
        return;
    }

    std::vector<disp_display_type> displays =
        mHasSecondaryPanel ? std::vector{MI_DISP_PRIMARY, MI_DISP_SECONDARY}
                           : std::vector{MI_DISP_PRIMARY};
    const disp_event_type notify_events[] = {
        MI_DISP_EVENT_POWER, MI_DISP_EVENT_FPS, MI_DISP_EVENT_51_BRIGHTNESS,
        MI_DISP_EVENT_HBM, MI_DISP_EVENT_DC};

    // Register for events
    for (const disp_display_type &display : displays) {
        for (const disp_event_type &event : notify_events) {
            disp_event_req req;
            req.base.flag = 0;
            req.base.disp_id = display;
            req.type = event;
            ioctl(mDispFd.get(), MI_DISP_IOCTL_REGISTER_EVENT, &req);
        }
    }

    struct pollfd dispEventPoll = {
        .fd = mDispFd.get(),
        .events = POLLIN,
    };

    _oem_msg *msg = new _oem_msg;
    notify_t notify_type;
    float value;

    while (mActive) {
        int rc = poll(&dispEventPoll, 1, -1);
        if (rc < 0) {
            LOG(ERROR) << "failed to poll " << DISP_FEATURE_PATH
                       << ", err: " << rc;
            continue;
        }

        struct disp_event_resp *response = parseDispEvent(mDispFd.get());
        if (response == nullptr) {
            continue;
        }

        switch (response->base.type) {
        case MI_DISP_EVENT_POWER:
            notify_type = POWER_STATE;
            value = response->data[0];
            break;
        case MI_DISP_EVENT_FPS:
            notify_type = DISPLAY_FREQUENCY;
            value = response->data[0];
            break;
        case MI_DISP_EVENT_51_BRIGHTNESS:
            notify_type = BRIGHTNESS;
            value = *(uint16_t *)response->data;
            break;
        case MI_DISP_EVENT_HBM:
            notify_type = BRIGHTNESS;
            value = response->data[0] ? -1 : -2;
            break;
        case MI_DISP_EVENT_DC:
            notify_type = DC_STATE;
            value = response->data[0];
            break;
        default:
            LOG(ERROR) << "got unknown event: " << response->base.type;
            continue;
        };

        msg->notify_type_float = msg->notify_type;

        const std::vector<int> &sensorsToNotify =
            (response->base.disp_id == MI_DISP_PRIMARY)
                ? mLightSensorsPrimary
                : mLightSensorsSecondary;

        for (const auto sensor_id : sensorsToNotify) {
            msg->sensor_type = sensor_id;
            msg->notify_type = notify_type;
            msg->notify_type_float = notify_type;
            msg->value = value;
            msg->unknown1 = 1;
            msg->unknown2 = 5;

            LOG(DEBUG) << "sending oem_msg for sensor " << msg->sensor_type
                       << " with type: " << msg->notify_type
                       << " and value: " << msg->value;
            mProcessMsg(msg);
        }
    }
}
