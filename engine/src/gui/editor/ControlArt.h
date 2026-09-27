// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdlib>
#include <string>

#include "hum/LayoutSpec.h"

namespace hum {

struct ControlArt {
    std::string image, on, down, track, arrow;
    int frames = 0;
    double fromAngle = -135.0, toAngle = 135.0;
    int slice = 0;
    std::string text;
    bool caption = true;

    bool empty() const { return image.empty(); }
};

namespace art {

inline std::string pathIn(const std::string& dir, const std::string& file) {
    if (file.empty() || dir.empty() || file.front() == '/') return file;
    return dir + "/" + file;
}

inline ControlArt forControl(const LayoutSpec::Control& c, const std::string& dir) {
    ControlArt a;
    a.image = pathIn(dir, c.extraOr("art"));
    if (a.image.empty()) return a;
    a.on = pathIn(dir, c.extraOr("art-on"));
    a.down = pathIn(dir, c.extraOr("art-down"));
    a.track = pathIn(dir, c.extraOr("art-track"));
    a.arrow = pathIn(dir, c.extraOr("art-arrow"));
    a.frames = std::atoi(c.extraOr("art-frames", "0").c_str());
    a.slice = std::atoi(c.extraOr("art-slice", "0").c_str());
    a.text = c.extraOr("art-text");
    a.caption = c.extraOr("art-caption", "true") != "false";
    const auto angles = c.extraOr("art-angles");
    if (const auto comma = angles.find(','); comma != std::string::npos) {
        a.fromAngle = std::atof(angles.substr(0, comma).c_str());
        a.toAngle = std::atof(angles.substr(comma + 1).c_str());
    }
    return a;
}

}

}
