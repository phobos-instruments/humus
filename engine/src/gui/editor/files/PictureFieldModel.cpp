// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/files/PictureFieldModel.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace hum::files {

PictureFieldModel::Look PictureFieldModel::lookNow() const {
    Look k;
    k.blur = pixelfield::blurRadiusFor(value(p_.blur));
    k.gate = (float) std::clamp(value(p_.gate), 0.0, 1.0);
    k.tilt = (float) std::clamp(value(p_.tilt), -1.0, 1.0);
    k.level = (float) std::clamp(value(p_.level), 0.0, 2.0);
    k.lowest = std::clamp(value(p_.lowest), 20.0, 2000.0);
    k.highest = std::max(k.lowest * 1.25, std::clamp(value(p_.highest), 200.0, 16000.0));
    k.y = std::clamp(value(p_.y), 0.0, 1.0);
    k.height = std::clamp(value(p_.height), 0.02, 1.0);
    return k;
}

float PictureFieldModel::tiltGain(const Look& k, int row, int height) {
    if (k.tilt == 0.0f || height <= 1) return 1.0f;
    const double rowTop = (1.0 - k.y - k.height) * (double) (height - 1);
    const double rowSpan = k.height * (double) (height - 1);
    if (rowSpan <= 0.0) return 1.0f;
    const double up = std::clamp(1.0 - ((double) row - rowTop) / rowSpan, 0.0, 1.0);
    const double hz = k.lowest * std::pow(k.highest / k.lowest, up);
    const double top = std::pow(k.highest / k.lowest, k.tilt * 0.5);
    const double g = std::pow(hz / k.lowest, k.tilt * 0.5);
    return (float) (k.tilt > 0.0f ? g / top : g);
}

PictureFieldModel::Reload PictureFieldModel::reload() {
    const auto path = host_.liveParamText(organism_, p_.picture);
    if (path == path_ && shownGen_ != kNoGeneration) return Reload::Unchanged;
    path_ = path;
    if (live::source<PixelFieldSource>(host_, organism_) != nullptr) {
        shownGen_ = kNoGeneration;
        return Reload::Live;
    }
    field_ = {};
    media_.loadPicture(path_, field_, kPictureWidth, kPictureHeight);
    return Reload::Loaded;
}

bool PictureFieldModel::pullLive() {
    auto* src = live::source<PixelFieldSource>(host_, organism_);
    if (src == nullptr) return false;
    const unsigned gen = src->pixelFieldGeneration();
    if (gen == shownGen_) return false;
    shownGen_ = gen;
    if (!src->copyPixelField(field_)) field_ = {};
    return true;
}

PictureFieldModel::Shade PictureFieldModel::shade() {
    look_ = lookNow();
    Shade out;
    if (field_.empty()) return out;
    PixelField shown;
    pixelfield::boxBlur(field_, look_.blur, shown);
    out.width = shown.width;
    out.height = shown.height;
    out.grey.resize((size_t) shown.width * (size_t) shown.height);
    for (int y = 0; y < shown.height; ++y) {
        const float rowGain = tiltGain(look_, y, shown.height) * look_.level;
        for (int x = 0; x < shown.width; ++x) {
            const float lit = pixelfield::gated(shown.at(x, y), look_.gate) * rowGain;
            out.grey[(size_t) y * (size_t) shown.width + (size_t) x] =
                (std::uint8_t) std::clamp((int) std::lround(lit * 255.0f), 0, 255);
        }
    }
    return out;
}

Rect PictureFieldModel::window(const Rect& area) const {
    const int x = area.x + (int) (value(p_.x) * area.w);
    const int w = std::max(2, (int) (value(p_.width) * area.w));
    const double y = value(p_.y), h = value(p_.height);
    const int top = area.y + (int) ((1.0 - y - h) * area.h);
    return {x, top, std::min(w, area.x + area.w - x), std::max(2, (int) (h * area.h))};
}

void PictureFieldModel::scrub(int px, int py, const Rect& area) {
    const double fx = std::clamp((double) (px - area.x) / area.w, 0.0, 1.0);
    const double fy = std::clamp(1.0 - (double) (py - area.y) / area.h, 0.0, 1.0);
    const double h = value(p_.height);
    host_.setParam(organism_, p_.x, fx);
    host_.setParam(organism_, p_.y, std::clamp(fy - h * 0.5, 0.0, 1.0 - h));
}

bool PictureFieldModel::followScan(int width) {
    float s = scan_, l = level_;
    const live::Controls controls(host_, organism_);
    s = controls.valueOr("scan", s);
    l = controls.valueOr("level", l);
    const int w = std::max(1, width);
    if ((int) (s * w) == (int) (scan_ * w) && std::abs(l - level_) < 0.02f) return false;
    scan_ = s;
    level_ = l;
    return true;
}

}
