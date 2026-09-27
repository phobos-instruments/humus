// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <string>
#include <utility>
#include <vector>

#include "core/net/ControlShape.h"
#include "core/xml/Xml.h"

namespace hum::controlxml {

inline constexpr double kFourteenBit = 16383.0;

inline ControlShape readBehaviour(const xml::Element& el) {
    ControlShape sh;
    sh.smoothing = el.doubleAttribute("smoothing", 0.0);
    if (el.hasAttribute("threshold")) {
        sh.type = ControlType::Button;
        sh.inverted = el.intAttribute("invert", 0) != 0;
        sh.button = el.intAttribute("toggle", 0) != 0 ? ButtonMode::Toggle : ButtonMode::Hold;
        sh.threshold = el.doubleAttribute("threshold", 8192.0) / kFourteenBit;
    }
    sh.type = parseMode(el.attribute("control-type", ""), sh.type);
    sh.button = parseMode(el.attribute("button-mode", ""), sh.button);
    sh.fader = parseMode(el.attribute("fader-mode", ""), sh.fader);
    sh.encoder = parseMode(el.attribute("encoding", ""), sh.encoder);
    sh.step = el.doubleAttribute("step", kDefaultControlStep);
    if (sh.isEncoder()) sh.inverted = el.intAttribute("invert", 0) != 0;
    return sh;
}

inline void readCurve(const xml::Element& owner, const char* tag, ControlShape& sh, bool dropIdentity) {
    auto* curve = owner.child(tag);
    if (curve == nullptr) return;
    for (auto* pt : curve->children())
        if (pt->hasTag("mapping-point"))
            sh.curve.push_back({pt->doubleAttribute("in", 0.0) / kFourteenBit,
                                pt->doubleAttribute("out", 0.0)});
    const bool identity = sh.curve.size() == 2
        && sh.curve[0] == std::make_pair(0.0, 0.0)
        && sh.curve[1] == std::make_pair(1.0, 1.0);
    if ((dropIdentity && identity) || sh.curve.size() < 2) sh.curve.clear();
}

inline void writeBehaviour(xml::Element& el, const ControlShape& sh) {
    el.setAttribute("control-type", modeWord(sh.type));
    if (sh.isButton()) {
        el.setAttribute("button-mode", modeWord(sh.button));
        el.setAttribute("invert", sh.inverted ? 1 : 0);
        el.setAttribute("toggle", sh.button == ButtonMode::Toggle ? 1 : 0);
        el.setAttribute("threshold", (int) std::lround(sh.threshold * kFourteenBit));
    }
    if (sh.isFader() && sh.fader != FaderMode::Direct)
        el.setAttribute("fader-mode", modeWord(sh.fader));
    if (sh.isEncoder()) {
        el.setAttribute("encoding", modeWord(sh.encoder));
        if (sh.inverted) el.setAttribute("invert", 1);
    }
    const bool stepped = sh.isEncoder() || (sh.isButton() && buttonModeUsesStep(sh.button));
    if (stepped) el.setAttribute("step", sh.step);
}

inline void writeCurve(xml::Element& owner, const char* tag,
                       const std::vector<std::pair<double, double>>& points) {
    auto* curve = owner.addChild(tag);
    for (const auto& p : points) {
        auto* pt = curve->addChild("mapping-point");
        pt->setAttribute("in", (int) std::lround(p.first * kFourteenBit));
        pt->setAttribute("out", p.second);
    }
}

}
