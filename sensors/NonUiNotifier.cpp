/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "NonUiNotifier"

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-base/unique_fd.h>

#include <android/looper.h>
#include <android/sensor.h>
#include <utils/Errors.h>

#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <string>

#include "xiaomi_touch.h"

using ::android::NO_ERROR;

#define SENSOR_TYPE_XIAOMI_SENSOR_NONUI 33171027
#define SENSOR_NAME_XIAOMI_SENSOR_NONUI "xiaomi.sensor.nonui"

#define TOUCH_DEV_PATH "/dev/xiaomi-touch"
#define TOUCH_MAGIC 'T'
#define TOUCH_IOC_SET_CUR_VALUE _IO(TOUCH_MAGIC, SET_CUR_VALUE)
#define TOUCH_IOC_GET_CUR_VALUE _IO(TOUCH_MAGIC, GET_CUR_VALUE)

namespace {

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

}  // namespace

struct SensorContext {
    ASensorEventQueue *queue;
};

int NonUiSensorCallback(__attribute__((unused)) int fd, __attribute__((unused)) int events,
                    void *data) {
    ASensorEvent event;
    int event_count = 0;
    SensorContext *context = reinterpret_cast<SensorContext *>(data);
    event_count = ASensorEventQueue_getEvents(context->queue, &event, 1);

    LOG(INFO) << "sensor reported: 0:" << event.data[0] << ", 1: " << event.data[1] << ", 2: " << event.data[2] << ", 3: " << event.data[3];

    return 1;
}

int main() {
    ASensorManager *sensorManager = nullptr;
    ASensorList sensorList;
    ASensorRef NonUiSensor = nullptr;
    ALooper *looper;
    struct SensorContext context = {nullptr};
    int err = NO_ERROR;

    sensorManager = ASensorManager_getInstanceForPackage("");
    if (!sensorManager) {
        LOG(ERROR) << "Failed to get ASensorManager instance";
        return 0;
    }

    size_t size = ASensorManager_getSensorList(sensorManager, &sensorList);
    for (int i = 0; i < size; i++) {
        if (strcmp(ASensor_getStringType(sensorList[i]), SENSOR_NAME_XIAOMI_SENSOR_NONUI) == 0 && ASensor_isWakeUpSensor(sensorList[i])) {
            NonUiSensor = sensorList[i];
            LOG(ERROR) << "found sensor: " << ASensor_getName(sensorList[i]) << " type: " << ASensor_getStringType(sensorList[i]) << " wakeup: " << ASensor_isWakeUpSensor(sensorList[i]) << " reporting mode: " << ASensor_getReportingMode(sensorList[i]);
            break;
        }
    }
    if (NonUiSensor == nullptr) {
        LOG(ERROR) << "Failed to get wake-up version of " << SENSOR_NAME_XIAOMI_SENSOR_NONUI;
        return 0;
    } else {
        looper = ALooper_forThread();
        if (looper == nullptr) {
            looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
        }
        context.queue =
            ASensorManager_createEventQueue(sensorManager, looper, 0, NonUiSensorCallback, &context);

        err = ASensorEventQueue_registerSensor(context.queue, NonUiSensor, 0, 0);
        if (err != NO_ERROR) {
            LOG(ERROR) << "Failed to register sensor with event queue, error: " << err;
            return 0;
        } else {
            while (true) {
                ALooper_pollOnce(-1, nullptr, nullptr, nullptr);
                LOG(ERROR) << "ALooper_pollOnce returned";
            }
        }
    }

    if (sensorManager != nullptr && context.queue != nullptr) {
        ASensorEventQueue_disableSensor(context.queue, NonUiSensor);
        ASensorManager_destroyEventQueue(sensorManager, context.queue);
    }

    return 0;
}
