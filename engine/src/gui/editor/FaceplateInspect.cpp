// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/Faceplate.h"

namespace hum {

double Faceplate::shownValue(const std::string& param) const {
    for (const auto& c : controls_)
        if (c.param == param && c.value) return c.value->shownValue();
    return -1.0;
}

float Faceplate::alphaOf(const std::string& param) const {
    for (const auto& c : controls_) {
        if (c.param != param) continue;
        if (c.rich) return c.rich->viewAlpha();
        if (c.momentary) return c.momentary->viewAlpha();
        if (c.combo) return c.combo->viewAlpha();
        if (c.value) return c.value->viewAlpha();
        if (c.toggle) return c.toggle->viewAlpha();
    }
    return 1.0f;
}

bool Faceplate::enabledOf(const std::string& param) const {
    for (const auto& c : controls_) {
        if (c.param != param) continue;
        if (c.value) return c.value->viewEnabled();
        if (c.combo) return c.combo->viewEnabled();
        if (c.rich) return c.rich->viewEnabled();
        if (c.toggle) return c.toggle->viewEnabled();
        if (c.momentary) return c.momentary->viewEnabled();
    }
    return true;
}

bool Faceplate::fileSlotShows(const std::string& param, const std::string& fileName) const {
    for (const auto& c : controls_)
        if (c.param == param && c.rich != nullptr && !c.rich->shownText().empty())
            return c.rich->shownText() == fileName;
    return false;
}

void Faceplate::stepCombo(const std::string& param, bool forward) {
    for (const auto& c : controls_)
        if (c.param == param) {
            if (c.combo && c.combo->onStep) c.combo->onStep(forward);
            return;
        }
}

ComboView* Faceplate::comboOf(const std::string& param) const {
    for (const auto& c : controls_)
        if (c.param == param && c.combo) return c.combo.get();
    return nullptr;
}

}
