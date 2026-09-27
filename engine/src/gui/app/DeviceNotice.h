// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/app/DeviceWatch.h"
#include "gui/app/TimedCard.h"
#include "gui/common/Localisation.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

class DeviceNotice : public TimedCard {
public:
    static constexpr double kLingerSeconds = 4.0;

    std::function<void()> onOpenSettings;
    explicit DeviceNotice(const DeviceChange& change) : change_(change) {
        open_.setButtonText(change.midi ? tr("device-notice.midi-settings", "MIDI Settings")
                                        : tr("device-notice.audio-settings", "Audio Settings"));
        open_.onClick = [this] { if (onOpenSettings) onOpenSettings(); };
        addAndMakeVisible(open_);
        setSize(kCardWidth, 96);
        expireIn(kLingerSeconds);
    }
    void paint(juce::Graphics& g) override {
        paintBody(g, true);
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
        g.drawText(change_.title, 14, 10, titleWidth(), 20, juce::Justification::centredLeft);
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(12.0f));
        g.drawFittedText(change_.message, 14, 32, getWidth() - 28, 18,
                         juce::Justification::centredLeft, 1);
    }
    void layout() override {
        placeButtons(buttonRow(), {&open_});
    }
private:
    DeviceChange change_;
    juce::TextButton open_;
};

}
