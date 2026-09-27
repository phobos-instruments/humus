// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/AutomateMenu.h"
#include "gui/host/BrickHost.h"
#include "gui/editor/inputs/PadInputs.h"
#include "gui/editor/OrganismEditor.h"

namespace hum {

class TapButton : public juce::TextButton {
public:
    TapButton(BrickHost& host, std::string organism, std::string param,
              std::string offParam, const juce::String& label)
        : juce::TextButton(label.isEmpty() ? "Tap" : label),
          tap_(host, std::move(organism), std::move(param), std::move(offParam)) {
        onClick = [this] { fire(); };
    }

private:
    void fire() { tap_.tap(juce::Time::getMillisecondCounterHiRes() * 0.001); }

    input::TapTempo tap_;
};

}
