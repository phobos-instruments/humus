#pragma once
#include <cmath>

#include "common/SsdDecode.h"

namespace hum {
namespace posedecode {

inline ssd::Config config() {
    ssd::Config c;
    c.inputSize = 224;
    c.strides[0] = 8; c.strides[1] = 16; c.strides[2] = 32;
    c.strides[3] = 32; c.strides[4] = 32;
    c.numLayers = 5;
    c.scale = 224.0f;
    c.rowWidth = 12;
    c.kpCount = 4;
    return c;
}

inline ssd::Crop cropFor(const ssd::Det& p) {
    const float dx = p.kp[1][0] - p.kp[0][0], dy = p.kp[1][1] - p.kp[0][1];
    const float angle = 3.14159265358979f * 0.5f - std::atan2(-dy, dx);
    const float size = 2.5f * std::hypot(dx, dy);
    return {p.kp[0][0], p.kp[0][1], size, angle};
}

inline ssd::Crop cropFromAlignment(float cx, float cy, float ax, float ay) {
    const float dx = ax - cx, dy = ay - cy;
    const float angle = 3.14159265358979f * 0.5f - std::atan2(-dy, dx);
    const float size = 2.5f * std::hypot(dx, dy);
    return {cx, cy, size, angle};
}

}
}
