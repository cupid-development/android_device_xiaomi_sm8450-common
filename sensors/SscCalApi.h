/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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

typedef void (*process_msg_t)(_oem_msg* msg);
typedef void (*init_current_sensors_t)(bool debug);
