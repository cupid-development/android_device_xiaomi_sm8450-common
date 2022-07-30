/*
 * Copyright (C) 2022 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "UdfpsHandler.xiaomi_sm8450"

#include "UdfpsHandler.h"

#include <android-base/logging.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <fstream>
#include <thread>

#define COMMAND_NIT 10
#define PARAM_NIT_FOD 1
#define PARAM_NIT_NONE 0

#define FOD_STATUS_PATH "/sys/devices/platform/soc/990000.spi/spi_master/spi0/spi0.0/fod_status"
#define FOD_STATUS_ON 1
#define FOD_STATUS_OFF 0

#define FOD_PRESSED_PATH "/sys/devices/platform/soc/990000.spi/spi_master/spi0/spi0.0/fod_down"

#define DISP_PARAM_PATH "/sys/devices/virtual/mi_display/disp_feature/disp-DSI-0/disp_param"
#define DISP_PARAM_LOCAL_HBM_ON "9 1"
#define DISP_PARAM_LOCAL_HBM_OFF "9 0"

template <typename T>
static void set(const std::string& path, const T& value) {
    std::ofstream file(path);
    file << value;
}

static bool readBool(int fd) {
    char c;
    int rc;

    rc = lseek(fd, 0, SEEK_SET);
    if (rc) {
        LOG(ERROR) << "failed to seek fd, err: " << rc;
        return false;
    }

    rc = read(fd, &c, sizeof(char));
    if (rc != 1) {
        LOG(ERROR) << "failed to read bool from fd, err: " << rc;
        return false;
    }

    return c != '0';
}

class XiaomiSm8450UdfpsHandler : public UdfpsHandler {
  public:
    void init(fingerprint_device_t* device) {
        LOG(INFO) << __func__;

        mDevice = device;

        std::thread([this]() {
            int fd = open(FOD_PRESSED_PATH, O_RDONLY);
            if (fd < 0) {
                LOG(ERROR) << "failed to open fd, err: " << fd;
                return;
            }

            struct pollfd fodUiPoll = {
                    .fd = fd,
                    .events = POLLERR | POLLPRI,
                    .revents = 0,
            };

            while (true) {
                int rc = poll(&fodUiPoll, 1, -1);
                if (rc < 0) {
                    LOG(ERROR) << "failed to poll fd, err: " << rc;
                    continue;
                }

                bool status = readBool(fd);

                LOG(INFO) << __func__ << " fod_pressed status: " << status;

                set(DISP_PARAM_PATH, status ? DISP_PARAM_LOCAL_HBM_ON : DISP_PARAM_LOCAL_HBM_OFF);
                mDevice->extCmd(mDevice, COMMAND_NIT, status ? PARAM_NIT_FOD : PARAM_NIT_NONE);
            }
        }).detach();
    }

    void onFingerDown(uint32_t /*x*/, uint32_t /*y*/, float /*minor*/, float /*major*/) {}

    void onFingerUp() {}

    void onAcquired(int32_t result, int32_t vendorCode) {
        LOG(INFO) << __func__ << " result: " << result << " vendorCode: " << vendorCode;
        // vendorCode 21 means waiting for fingerprint
        // result 0 means fingerprint detected successfully
        if (vendorCode == 21 || vendorCode == 22 || vendorCode == 23) {
            set(FOD_STATUS_PATH, FOD_STATUS_ON);
        } else if (result == 0) {
            set(FOD_STATUS_PATH, FOD_STATUS_OFF);
        }
    }

    void cancel() {
        LOG(INFO) << __func__;
        set(FOD_STATUS_PATH, FOD_STATUS_OFF);
    }

  private:
    fingerprint_device_t* mDevice;
};

static UdfpsHandler* create() {
    return new XiaomiSm8450UdfpsHandler();
}

static void destroy(UdfpsHandler* handler) {
    delete handler;
}

extern "C" UdfpsHandlerFactory UDFPS_HANDLER_FACTORY = {
        .create = create,
        .destroy = destroy,
};
