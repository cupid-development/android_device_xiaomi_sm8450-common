/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "DisplayInfo2SlpiNotifier.xiaomi_sm8450"

#include <android-base/logging.h>
#include <android-base/properties.h>

#include <fcntl.h>
#include <poll.h>
#include <string>

#include <display/drm/mi_disp.h>

#define DISP_FEATURE_PATH "/dev/mi_display/disp_feature"

#define SENSOR_TYPE_LIGHT 5
#define SENSOR_TYPE_GS1602_CCT_STREAM 33171089

enum notify_t {
    BRIGHTNESS = 17,
    DC_STATE = 18,
    DISPLAY_FREQUENCY = 20,
    REPORT_VALUE = 201,
    POWER_STATE = 202,
};

struct _oem_msg {
    uint32_t sensor_type;
    notify_t notify_type;
    float unknown1;
    float unknown2;
    float notify_type_float;
    float value;
    float unused0;
    float unused1;
    float unused2;
};

extern void process_msg(_oem_msg* msg);
extern void init_current_sensors(bool debug);

namespace {

static disp_event_resp* parseDispEvent(int fd) {
    disp_event header;
    ssize_t headerSize = read(fd, &header, sizeof(header));
    if (headerSize < sizeof(header)) {
        LOG(ERROR) << "unexpected display event header size: " << headerSize;
        return nullptr;
    }

    struct disp_event_resp* response =
            reinterpret_cast<struct disp_event_resp*>(malloc(header.length));
    response->base = header;

    int dataLength = response->base.length - sizeof(response->base);
    if (dataLength < 0) {
        LOG(ERROR) << "invalid data length: " << response->base.length;
        return nullptr;
    }

    ssize_t dataSize = read(fd, &response->data, dataLength);
    if (dataSize < dataLength) {
        LOG(ERROR) << "unexpected display event data size: " << dataSize
                   << ", expected: " << dataLength;
        return nullptr;
    }

    return response;
}

}  // namespace

int main() {
    bool debug_enable = android::base::GetBoolProperty("persist.vendor.debug.ssccalapi", false);
    init_current_sensors(debug_enable);

    // Thread to listen for display changes
    int fd = open(DISP_FEATURE_PATH, O_RDWR);
    if (fd < 0) {
        LOG(ERROR) << "failed to open " << DISP_FEATURE_PATH << " , err: " << fd;
        return -EIO;
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
        ioctl(fd, MI_DISP_IOCTL_REGISTER_EVENT, &req);
    }

    struct pollfd dispEventPoll = {
            .fd = fd,
            .events = POLLIN,
            .revents = 0,
    };

    _oem_msg* msg = new _oem_msg;

    while (true) {
        int rc = poll(&dispEventPoll, 1, -1);
        if (rc < 0) {
            LOG(ERROR) << "failed to poll " << DISP_FEATURE_PATH << ", err: " << rc;
            continue;
        }

        struct disp_event_resp* response = parseDispEvent(fd);
        if (response == nullptr) {
            continue;
        }

        switch (response->base.type) {
            case MI_DISP_EVENT_POWER:
                msg->notify_type = POWER_STATE;
                msg->value = response->data[0];
                break;
            case MI_DISP_EVENT_FPS:
                msg->notify_type = DISPLAY_FREQUENCY;
                msg->value = response->data[0];
                break;
            case MI_DISP_EVENT_51_BRIGHTNESS:
                msg->notify_type = BRIGHTNESS;
                msg->value = *(uint16_t*)response->data;
                break;
            case MI_DISP_EVENT_HBM:
                msg->notify_type = BRIGHTNESS;
                msg->value = response->data[0] ? -1 : -2;
                break;
            case MI_DISP_EVENT_DC:
                msg->notify_type = DC_STATE;
                msg->value = response->data[0];
                break;
            default:
                LOG(ERROR) << "got unknown event: " << response->base.type;
                continue;
        };

        msg->notify_type_float = msg->notify_type;
        msg->unknown1 = 1;
        msg->unknown2 = 5;

        for (const auto sensor_id : {SENSOR_TYPE_LIGHT, SENSOR_TYPE_GS1602_CCT_STREAM}) {
            msg->sensor_type = sensor_id;
            LOG(DEBUG) << "sending oem_msg for sensor " << msg->sensor_type
                       << " with type: " << msg->notify_type << " and value: " << msg->value;
            process_msg(msg);
        }
    }

    // Should never reach this
    return EXIT_SUCCESS;
}
