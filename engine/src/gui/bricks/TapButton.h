// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/timeline/TapTempo.h"
#include "gui/editor/AutomateMenu.h"
#include "gui/host/BrickHost.h"
#include "io/PatchDocument.h"
#include "gui/editor/OrganismEditor.h"

namespace hum {

class TapButton : public juce::TextButton {
public:
    TapButton(BrickHost& host, std::string organism, std::string param,
              std::string offParam, const juce::String& label)
        : juce::TextButton(label.isEmpty() ? "Tap" : label),
          host_(host), organism_(std::move(organism)), param_(std::move(param)),
          offParam_(std::move(offParam)) {
        onClick = [this] { fire(); };
    }

private:
    void fire() {
        const double ms = core_.tap(juce::Time::getMillisecondCounterHiRes() * 0.001);
        if (ms <= 0.0) return;
        if (!offParam_.empty()) {
            double cur = 0.0;
            if (const auto* cm = host_.model().byName(organism_))
                for (const auto& pr : cm->properties)
                    if (pr.name == offParam_) { cur = pr.value; break; }
            if (cur >= 0.5) {
                host_.pushUndo();
                host_.setParam(organism_, offParam_, 0.0);
            }
        }
        const auto range = paramRange(host_, organism_, param_);
        host_.editParam(organism_, param_,
                        juce::jlimit(range.first, range.second, ms));
    }

    BrickHost& host_;
    std::string organism_, param_, offParam_;
    TapTempoCore core_;
};

}
