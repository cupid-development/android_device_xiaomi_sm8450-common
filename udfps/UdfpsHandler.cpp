/*
 * Copyright (C) 2022 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "UdfpsHandler.xiaomi_sm8450"

#include "UdfpsHandler.h"

#include <android-base/logging.h>
#include <android-base/unique_fd.h>
#include <fcntl.h>
#include <poll.h>
#include <thread>
#include <unistd.h>

#define THP_CMD_BASE	1000

enum MODE_TYPE {
    Touch_Game_Mode				= 0,
    Touch_Active_MODE      		= 1,
    Touch_UP_THRESHOLD			= 2,
    Touch_Tolerance				= 3,
    Touch_Aim_Sensitivity       = 4,
    Touch_Tap_Stability         = 5,
    Touch_Expert_Mode           = 6,
    Touch_Edge_Filter      		= 7,
    Touch_Panel_Orientation 	= 8,
    Touch_Report_Rate      		= 9,
    Touch_Fod_Enable       		= 10,
    Touch_Aod_Enable       		= 11,
    Touch_Resist_RF        		= 12,
    Touch_Idle_Time        		= 13,
    Touch_Doubletap_Mode   		= 14,
    Touch_Grip_Mode        		= 15,
    Touch_FodIcon_Enable   		= 16,
    Touch_Nonui_Mode       		= 17,
    Touch_Debug_Level      		= 18,
    Touch_Power_Status     		= 19,
    Touch_Mode_NUM         		= 20,
    THP_LOCK_SCAN_MODE      	= THP_CMD_BASE + 0,
    THP_FOD_DOWNUP		      	= THP_CMD_BASE + 1,
    THP_SET_REPORT_RATE      	= THP_CMD_BASE + 11,
};

#define COMMAND_NIT 10
#define PARAM_NIT_FOD 1
#define PARAM_NIT_NONE 0

#define FOD_STATUS_ON 1
#define FOD_STATUS_OFF 0

// defines from drivers/input/touchscreen/xiaomi/xiaomi_touch.h
#define SET_CUR_VALUE 0
#define Touch_Fod_Enable 10

#define TOUCH_DEV_PATH "/dev/xiaomi-touch"
#define TOUCH_ID 0
#define TOUCH_MAGIC 0x5400
#define TOUCH_IOC_SETMODE TOUCH_MAGIC + SET_CUR_VALUE

#define FOD_PRESS_STATUS_PATH "/sys/devices/virtual/touch/touch_dev/fod_press_status"

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

class XiaomiSm8450UdfpsHander : public UdfpsHandler {
  public:
    void init(fingerprint_device_t *device) {
        mDevice = device;
        touch_fd_ = android::base::unique_fd(open(TOUCH_DEV_PATH, O_RDWR));

        std::thread([this]() {
            int fd = open(FOD_PRESS_STATUS_PATH, O_RDONLY);
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

                mDevice->extCmd(mDevice, COMMAND_NIT,
                                readBool(fd) ? PARAM_NIT_FOD : PARAM_NIT_NONE);

                int arg[3] = {TOUCH_ID, THP_FOD_DOWNUP, readBool(fd) ? 1 : 0};
                ioctl(touch_fd_.get(), TOUCH_IOC_SETMODE, &arg);

            }
        }).detach();
    }

    void onFingerDown(uint32_t /*x*/, uint32_t /*y*/, float /*minor*/, float /*major*/) {
        // nothing
    }

    void onFingerUp() {
        // nothing
    }

    void onAcquired(int32_t result, int32_t vendorCode) {
        LOG(INFO) << __func__ << " result: " << result << " vendorCode: " << vendorCode;
        // vendorCode 21 means waiting for fingerprint
        // result 0 means fingerprint detected successfully
        if (vendorCode == 21 || vendorCode == 22 || vendorCode == 23) {
            int arg[3] = {TOUCH_ID, Touch_Fod_Enable, FOD_STATUS_ON};
            ioctl(touch_fd_.get(), TOUCH_IOC_SETMODE, &arg);
        } else if (result == 0) {
            int arg[3] = {TOUCH_ID, Touch_Fod_Enable, FOD_STATUS_OFF};
            ioctl(touch_fd_.get(), TOUCH_IOC_SETMODE, &arg);
        }
    }

    void cancel() {
        LOG(INFO) << __func__;
        int arg[3] = {TOUCH_ID, Touch_Fod_Enable, FOD_STATUS_OFF};
        ioctl(touch_fd_.get(), TOUCH_IOC_SETMODE, &arg);
    }

  private:
    fingerprint_device_t *mDevice;
    android::base::unique_fd touch_fd_;
};

static UdfpsHandler* create() {
    return new XiaomiSm8450UdfpsHander();
}

static void destroy(UdfpsHandler* handler) {
    delete handler;
}

extern "C" UdfpsHandlerFactory UDFPS_HANDLER_FACTORY = {
    .create = create,
    .destroy = destroy,
};
