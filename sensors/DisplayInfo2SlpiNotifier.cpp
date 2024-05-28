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

// using vendor::display::config::V2_0::IDisplayConfig;
// using vendor::display::config::V2_0::IDisplayConfigCallback;

#include <config/client_interface.h>
#include <gr_priv_handle.h>
#include <hardware/gralloc.h>

using namespace android;
using namespace DisplayConfig;

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

#define NSEC_TO_SEC (1000000000l)

float getCurrentFps() {
    float fps = 0;
    int ret = 0;
    uint32_t dpy_index = -1;
    ClientInterface* client = nullptr;
    Attributes dpy_attr;
    char buf[150];

    ret = DisplayConfig::ClientInterface::Create("disp2slpi", nullptr, &client);

    if (client != NULL) {
        client->GetActiveConfig(DisplayConfig::DisplayType::kPrimary, &dpy_index);
        if (dpy_index >= 0) {
            ret = client->GetDisplayAttributes(dpy_index, DisplayConfig::DisplayType::kPrimary,
                                               &dpy_attr);
            if (ret == 0 && dpy_attr.vsync_period) {
                LOG(ERROR) << "VSYNC : " << dpy_attr.vsync_period;
                fps = NSEC_TO_SEC / dpy_attr.vsync_period;
                LOG(ERROR) << "Panel Attributes vsyncPeriod: " << dpy_attr.vsync_period
                           << " Fps: " << fps;
            }
        }
        ClientInterface::Destroy(client);
    }

    return fps;
}

}  // namespace

class CWBCallback : public ConfigCallback {
  public:
    void NotifyCWBBufferDone(int error, const native_handle_t* buffer) override {
        LOG(ERROR) << "NotifyCWBBufferDone called with error code: " << error;
    }

    void NotifyQsyncChange(bool qsync_enabled, int refresh_rate, int qsync_refresh_rate) override {
        LOG(ERROR) << "NotifyQsyncChange called with qsync_enabled: " << qsync_enabled
                   << ", refresh_rate: " << refresh_rate
                   << ", qsync_refresh_rate: " << qsync_refresh_rate;
    }

    void NotifyIdleStatus(bool is_idle) override {
        LOG(ERROR) << "NotifyIdleStatus called with is_idle: " << is_idle;
    }
};

int main() {
    ClientInterface* client = nullptr;
    CWBCallback callback;
    int ret = 0;
    const uint32_t disp_id = static_cast<uint32_t>(DisplayType::kPrimary);
    native_handle_t* buffer = native_handle_create(1, 1);
    
    // private_handle_t phandle = new private_handle_t(int fd, int meta_fd, int flags, int width, int height, int uw, int uh,
    //                int format, int buf_type, unsigned int size, uint64_t usage = 0)


    // TODO: get rect from cwbInfo in lightSensorConfig.json
    const Rect rect = {
            .left = 697,
            .top = 3,
            .right = 793,
            .bottom = 99,
    };


    bool is_wb_ubwc_supported = false;

    BufferInfo output_buffer_info;
    // Initialize CWB buffer with display resolution to get full size buffer
    // as mixer or fb can init with custom values based on property
    output_buffer_info.buffer_config.width = rect.left - rect.right;
    output_buffer_info.buffer_config.height = rect.bottom - rect.top;
    if (is_wb_ubwc_supported) {
        output_buffer_info.buffer_config.format = 13U; //kFormatRGBX8888Ubwc
    } else {
        output_buffer_info.buffer_config.format = 8U; //kFormatRGB888
    }
    output_buffer_info.buffer_config.buffer_count = 1;
    if (buffer_allocator_->AllocateBuffer(&output_buffer_info) != 0) {
        DLOGE("Buffer allocation failed");
        return;
    }

    LayerBuffer buffer = {};
    buffer.planes[0].fd = output_buffer_info.alloc_buffer_info.fd;
    buffer.planes[0].offset = 0;
    buffer.planes[0].stride = output_buffer_info.alloc_buffer_info.stride;
    buffer.size = output_buffer_info.alloc_buffer_info.size;
    buffer.handle_id = output_buffer_info.alloc_buffer_info.id;
    buffer.width = output_buffer_info.alloc_buffer_info.aligned_width;
    buffer.height = output_buffer_info.alloc_buffer_info.aligned_height;
    buffer.format = output_buffer_info.alloc_buffer_info.format;
    buffer.unaligned_width = output_buffer_info.buffer_config.width;
    buffer.unaligned_height = output_buffer_info.buffer_config.height;

    cwb_layer_.composition = kCompositionCWBTarget;
    cwb_layer_.input_buffer = buffer;
    cwb_layer_.input_buffer.buffer_id = reinterpret_cast<uint64_t>(output_buffer_info.private_data);
    cwb_layer_.src_rect = {0, 0, FLOAT(cwb_layer_.input_buffer.unaligned_width),
                            FLOAT(cwb_layer_.input_buffer.unaligned_height)};
    cwb_layer_.dst_rect = {0, 0, FLOAT(cwb_layer_.input_buffer.unaligned_width),
                            FLOAT(cwb_layer_.input_buffer.unaligned_height)};

    cwb_layer_.flags.is_cwb = 1;

    CwbConfig cwb_config = {};
    cwb_config.tap_point = CwbTapPoint::kLmTapPoint;
    cwb_config.cwb_full_rect = LayerRect(0.0f, 0.0f, FLOAT(cwb_layer_.input_buffer.width),
                                        FLOAT(cwb_layer_.input_buffer.height));
    cwb_buffer_inited_ = true;



    ret = ClientInterface::Create("disp2slpi", &callback, &client);
    if (ret < 0) {
        LOG(ERROR) << "failed to create ClientInterface, ret: " << ret;
    }


    ret = client->SetCWBOutputBuffer(disp_id, rect, true, &phandle);

    while (true) {
        sleep(1);
    }

    ClientInterface::Destroy(client);

    return 0;
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
