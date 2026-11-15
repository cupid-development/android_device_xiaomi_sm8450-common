/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */
#include "Compensation.h"
#include "Timeline.h"

#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 3 || argc > 4) return 2;
    udals::Compensation model;
    std::string error;
    if (!model.load(argv[1], argv[2], argc == 4 ? argv[3] : "", &error)) {
        std::cerr << error << '\n';
        return 1;
    }
    udals::Timeline timeline;
    timeline.display({1000000000, 100, false, true});
    timeline.display({1100000000, 200, false, true});
    timeline.sensor({1110000000, {1, 2, 3, 4}});
    auto sample = timeline.align(1105000000);
    assert(sample && sample->display.dbv == 100);
    assert(!timeline.align(1105000000));
    timeline.sensor({1130000000, {1, 2, 3, 4}});
    sample = timeline.align(1120000000);
    assert(sample && sample->display.dbv == 200);
    timeline.clear();
    assert(!timeline.align(1120000000));
    timeline.display({1200000000, 0, false, false});
    timeline.sensor({1230000000, {1, 2, 3, 4}});
    assert(!timeline.align(1220000000));
    timeline.display({2000000000, 100, false, true});
    timeline.sensor({4000000000, {1, 2, 3, 4}});
    assert(!timeline.align(2100000000));

    if (model.isMarble()) {
        assert(model.filter(10) == 10);
        assert(model.filter(20) == 20);
        assert(model.filter(100) == 20);
        model.reset();
        model.reported(100);
        assert(!model.filter(103));
        assert(model.filter(105) == 105);
    } else {
        assert(model.filter(1) == 1);
        assert(model.filter(3) == 1);
        assert(model.filter(5) == 2);
        model.reset();
        for (float value : {100.1f, 200.2f, 300.3f, 400.4f}) assert(model.filter(value) == value);
        assert(model.filter(500.5f) == 300);
        model.reset();
        model.reported(5);
        assert(!model.filter(8));
        model.reset();
        model.reported(5);
        assert(model.filter(10) == 10);
    }
    model.reset();

    const auto region = model.region();
    const size_t stride = region.width + 11;
    std::vector<uint8_t> pixels(stride * (region.height + 2) * 4, 255);
    for (int y = 0; y < region.height; ++y) {
        for (int x = 0; x < region.width; ++x) {
            for (int c = 0; c < 3; ++c) pixels[((y + 2) * stride + x + 3) * 4 + c] = 0;
        }
    }
    assert((model.sample(pixels.data(), stride, region.left - 3, region.top - 2) == udals::Rgb{}));
    const std::array<int, 3> color{128, 64, 32};
    for (int y = 0; y < region.height; ++y) {
        for (int x = 0; x < region.width; ++x) {
            for (int c = 0; c < 3; ++c) pixels[((y + 2) * stride + x + 3) * 4 + c] = color[c];
        }
    }
    auto rgbSample = model.sample(pixels.data(), stride, region.left - 3, region.top - 2);
    const udals::Rgb weights = model.isMarble()
                                       ? udals::Rgb{12.26136428, 22.821466596, 10.000005812}
                                       : udals::Rgb{18.686293052, 18.686293052, 18.686293052};
    for (size_t c = 0; c < 3; ++c) {
        double expected = std::pow(color[c], 2.2) * 25 * weights[c];
        assert(std::abs(rgbSample[c] - expected) <= expected * 1e-11);
    }
    std::cout << std::setprecision(17);
    int dbv, dc;
    udals::Rgb rgb;
    udals::Channels raw;
    while (std::cin >> dbv >> dc >> rgb[0] >> rgb[1] >> rgb[2] >> raw[0] >> raw[1] >> raw[2] >>
           raw[3]) {
        auto bleed = model.predict(dbv, dc, rgb);
        for (double value : bleed) std::cout << value << ' ';
        std::cout << model.lux(raw, bleed, dbv) << '\n';
    }
}
