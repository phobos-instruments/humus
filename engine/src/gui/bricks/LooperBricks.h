// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/AutomateMenu.h"
#include "gui/style/Colours.h"
#include "gui/style/IconGlyph.h"
#include "gui/host/BrickHost.h"
#include "gui/host/EngineHostFiles.h"
#include "gui/style/LookAndFeel.h"
#include "gui/editor/Mappable.h"
#include "gui/bricks/MomentaryButton.h"
#include "gui/editor/MomentaryPress.h"
#include "gui/editor/inputs/RecorderInputs.h"
#include "gui/editor/OrganismEditor.h"

namespace hum {

class LooperTrackStrip : public juce::Component, private juce::Timer {
public:
    LooperTrackStrip(BrickHost& host, std::string organism, std::string prefix, int count)
        : host_(host), name_(organism), tracks_(host, organism, std::move(prefix), count) {
        for (int i = 0; i < count; ++i) {
            auto t = std::make_unique<Mappable<juce::ToggleButton>>(juce::String(i + 1));
            t->setColour(juce::ToggleButton::textColourId, Palette::text);
            t->setColour(juce::ToggleButton::tickColourId, Palette::accent);
            const int idx = i;
            t->onClick = [this, idx] {
                tracks_.set(idx, toggles_[(size_t) idx]->getToggleState());
            };
            t->onRightClick = [this, idx](juce::Point<int> pos) {
                showAutomateMenu(host_, name_, tracks_.param(idx), pos, nullptr);
            };
            addAndMakeVisible(*t);
            toggles_.push_back(std::move(t));
        }
        reload();
        startTimerHz(15);
    }
    ~LooperTrackStrip() override { stopTimer(); }

    void reload() {
        for (size_t i = 0; i < toggles_.size(); ++i)
            toggles_[i]->setToggleState(tracks_.on((int) i), juce::dontSendNotification);
    }

    void resized() override {
        auto r = getLocalBounds();
        const int rows = std::max(1, ((int) toggles_.size() + 7) / 8);
        const int rowH = r.getHeight() / rows;
        for (int row = 0; row < rows; ++row) {
            auto rr = r.removeFromTop(rowH);
            const int tw = rr.getWidth() / 8;
            for (int col = 0; col < 8; ++col) {
                const size_t idx = (size_t) (row * 8 + col);
                if (idx < toggles_.size()) toggles_[idx]->setBounds(rr.removeFromLeft(tw));
            }
        }
    }

private:
    void timerCallback() override {
        int rec = -1, armed = -1;
        host_.files().liveLooperTracks(name_, rec, armed);
        if (!tracks_.follow(rec, armed)) return;
        for (int i = 0; i < (int) toggles_.size(); ++i) {
            const juce::Colour c = tracks_.recording(i) ? ink::state::danger
                                 : tracks_.armed(i)     ? Palette::accent
                                                        : Palette::text;
            toggles_[(size_t) i]->setColour(juce::ToggleButton::textColourId, c);
            toggles_[(size_t) i]->repaint();
        }
    }

    BrickHost& host_;
    std::string name_;
    input::LooperTracks tracks_;
    std::vector<std::unique_ptr<juce::ToggleButton>> toggles_;
};

}
