// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "gui/editor/readouts/ControlReadings.h"
#include "gui/editor/Geometry.h"
#include "gui/editor/LiveControls.h"
#include "gui/editor/files/MediaLibrary.h"
#include "gui/host/ModelHost.h"
#include "hum/PixelField.h"
#include "hum/caps/Video.h"

namespace hum::files {

struct PictureParams {
    std::string picture, blur, gate, tilt, level, lowest, highest, x, y, width, height;
};

class PictureFieldModel {
public:
    struct Look {
        int blur = 0;
        float gate = 0.0f, tilt = 0.0f, level = 1.0f;
        double lowest = 55.0, highest = 8000.0, y = 0.0, height = 1.0;
        bool operator==(const Look& o) const {
            return blur == o.blur && gate == o.gate && tilt == o.tilt && level == o.level && lowest == o.lowest
                   && highest == o.highest && y == o.y && height == o.height;
        }
        bool operator!=(const Look& o) const { return !(*this == o); }
    };

    struct Shade {
        int width = 0, height = 0;
        std::vector<std::uint8_t> grey;
    };

    static constexpr unsigned kNoGeneration = ~0u;
    enum class Reload { Unchanged, Live, Loaded };

    static constexpr int kPictureWidth = 512, kPictureHeight = 256;

    PictureFieldModel(ModelHost& host, MediaLibrary& media, std::string organism, PictureParams params)
        : host_(host), media_(media), organism_(std::move(organism)), p_(std::move(params)) {}

    double value(const std::string& param) const { return host_.liveParamValue(organism_, param); }

    Look lookNow() const;
    static float tiltGain(const Look& k, int row, int height);

    bool empty() const { return field_.empty(); }
    const Look& look() const { return look_; }

    Reload reload();
    bool pullLive();

    bool lookMoved() const { return !field_.empty() && lookNow() != look_; }

    Shade shade();
    Rect window(const Rect& area) const;
    void scrub(int px, int py, const Rect& area);
    bool followScan(int width);

    float scan() const { return scan_; }
    float level() const { return level_; }

    void setPicture(const std::string& path) { host_.setParamText(organism_, p_.picture, path); }

private:
    ModelHost& host_;
    MediaLibrary& media_;
    std::string organism_;
    PictureParams p_;
    std::string path_;
    PixelField field_;
    unsigned shownGen_ = kNoGeneration;
    Look look_;
    float scan_ = 0.0f, level_ = 0.0f;
};

}
