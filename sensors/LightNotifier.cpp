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

using android::hardware::Return;
using android::hardware::Void;
using android::hardware::sensors::V1_0::Event;

LightNotifier::LightNotifier(sp<ISensorManager> manager, process_msg_t processMsg)
    : SensorNotifier(manager, processMsg) {
    std::stringstream light_sensors(
            android::base::GetProperty("ro.vendor.sensors.notifier.light_sensors", ""));
    std::string sensor;

    while (std::getline(light_sensors, sensor, ',')) {
        mLightSensors.push_back(std::stoi(sensor));
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

    android::base::unique_fd disp_fd_ = android::base::unique_fd(open(DISP_FEATURE_PATH, O_RDWR));
    if (disp_fd_.get() == -1) {
        LOG(ERROR) << "failed to open " << DISP_FEATURE_PATH;
    }

    const disp_event_type notify_events[] = {MI_DISP_EVENT_POWER, MI_DISP_EVENT_FPS,
                                             MI_DISP_EVENT_51_BRIGHTNESS, MI_DISP_EVENT_HBM,
                                             MI_DISP_EVENT_DC};

    // Register for events
    for (const disp_event_type& event : notify_events) {
        disp_event_req req;
        req.base.flag = 0;
        req.base.disp_id = MI_DISP_PRIMARY;
        req.type = event;
        ioctl(disp_fd_.get(), MI_DISP_IOCTL_REGISTER_EVENT, &req);
    }

    struct pollfd dispEventPoll = {
            .fd = disp_fd_.get(),
            .events = POLLIN,
    };

    _oem_msg* msg = new _oem_msg;
    notify_t notify_type;
    float value;

    while (mActive) {
        int rc = poll(&dispEventPoll, 1, -1);
        if (rc < 0) {
            LOG(ERROR) << "failed to poll " << DISP_FEATURE_PATH << ", err: " << rc;
            continue;
        }

        struct disp_event_resp* response = parseDispEvent(disp_fd_.get());
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
                value = *(uint16_t*)response->data;
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

        for (const auto sensor_id : mLightSensors) {
            msg->sensor_type = sensor_id;
            msg->notify_type = notify_type;
            msg->notify_type_float = notify_type;
            msg->value = value;
            msg->unknown1 = 1;
            msg->unknown2 = 5;

            LOG(DEBUG) << "sending oem_msg for sensor " << msg->sensor_type
                      << " with type: " << msg->notify_type << " and value: " << msg->value;
            mProcessMsg(msg);
        }
    }
}
