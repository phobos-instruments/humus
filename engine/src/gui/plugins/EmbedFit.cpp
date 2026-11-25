// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/plugins/EmbedFit.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace hum {

void EmbedFit::reset(Size editor) {
    natural_ = reference_ = editor;
    rebaseline_ = false;
    held_ = 0.0f;
    ignoresZoom_ = false;
    appliedAtRead_ = outerAtRead_ = 1.0f;
    settle_ = 0;
}

bool EmbedFit::hostChanged(float appliedScale, float outerScale) {
    if (std::abs(appliedScale - appliedAtRead_) <= 0.0005f && std::abs(outerScale - outerAtRead_) <= 0.0005f)
        return false;
    appliedAtRead_ = appliedScale;
    outerAtRead_ = outerScale;
    settle_ = kSettleTicks;
    rebaseline_ = true;
    return true;
}

bool EmbedFit::closeToNatural(Size seen, float toPlugin) const {
    const int rounding = 2 + (int) std::ceil(2.0f * toPlugin);
    const auto near = [rounding](int a, int b) { return std::abs(a - b) <= std::max(rounding, b / 33); };
    return near(seen.w, natural_.w) && near(seen.h, natural_.h);
}

bool EmbedFit::learn(Size natural, bool buttonDown, float shownScale) {
    if (buttonDown && held_ <= 0.0f) held_ = shownScale;
    natural_ = natural;
    return true;
}

bool EmbedFit::observe(Size editor, float appliedScale, float outerScale, bool buttonDown, float shownScale) {
    if (appliedScale <= 0.0f || outerScale <= 0.0f) return false;
    if (hostChanged(appliedScale, outerScale)) return false;
    if (settle_ > 0) {
        --settle_;
        return false;
    }
    const float toPlugin = outerScale / appliedScale;
    if (rebaseline_) {
        rebaseline_ = false;
        reference_ = editor;
        const Size seen{(int) std::lround(editor.w * toPlugin), (int) std::lround(editor.h * toPlugin)};
        if (closeToNatural(seen, toPlugin)) return false;
        const Size unzoomed{(int) std::lround(editor.w * outerScale), (int) std::lround(editor.h * outerScale)};
        if (std::abs(appliedScale - outerScale) > 0.01f && closeToNatural(unzoomed, outerScale)) {
            ignoresZoom_ = true;
            return false;
        }
        return learn(seen, buttonDown, shownScale);
    }
    const int dw = editor.w - reference_.w, dh = editor.h - reference_.h;
    if (std::abs(dw) <= 1 && std::abs(dh) <= 1) return false;
    reference_ = editor;
    return learn({natural_.w + (int) std::lround(dw * toPlugin), natural_.h + (int) std::lround(dh * toPlugin)},
                 buttonDown, shownScale);
}

void EmbedFit::freezeAt(float shownScale) {
    if (held_ <= 0.0f && shownScale > 0.0f) held_ = shownScale;
}

bool EmbedFit::letGo(bool buttonDown, int boxWidth) {
    if (held_ <= 0.0f || buttonDown) return false;
    const float was = held_;
    held_ = 0.0f;
    return std::abs(scale(boxWidth) - was) > 0.0005f;
}

float EmbedFit::scale(int boxWidth) const {
    if (held_ > 0.0f) return held_;
    return natural_.w > 0 && boxWidth > 0 ? (float) boxWidth / (float) natural_.w : 1.0f;
}

int EmbedFit::heightFor(int boxWidth) const {
    return std::max(24, (int) std::lround(natural_.h * scale(boxWidth)));
}

}
