// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/app/DeviceWatch.h"
#include "gui/common/Localisation.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

class DeviceNotice : public juce::Component {
public:
    std::function<void()> onOpenSettings, onIgnore;
    explicit DeviceNotice(const DeviceChange& change) : change_(change) {
        open_.setButtonText(change.midi ? tr("device-notice.midi-settings", "MIDI Settings")
                                        : tr("device-notice.audio-settings", "Audio Settings"));
        ignore_.setButtonText(tr("device-notice.ignore", "Ignore"));
        open_.onClick = [this] { if (onOpenSettings) onOpenSettings(); };
        ignore_.onClick = [this] { if (onIgnore) onIgnore(); };
        addAndMakeVisible(open_);
        addAndMakeVisible(ignore_);
        setSize(348, 96);
    }
    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(Palette::panel);
        g.fillRoundedRectangle(r, 6.0f);
        g.setColour(Palette::accent.withAlpha(alpha::strong));
        g.drawRoundedRectangle(r, 6.0f, 1.2f);
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
        g.drawText(change_.title, 14, 10, getWidth() - 28, 20, juce::Justification::centredLeft);
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(12.0f));
        g.drawFittedText(change_.message, 14, 32, getWidth() - 28, 18,
                         juce::Justification::centredLeft, 1);
    }
    void resized() override {
        auto row = getLocalBounds().reduced(12).removeFromBottom(26);
        open_.setBounds(row.removeFromLeft(118));
        row.removeFromLeft(8);
        ignore_.setBounds(row.removeFromLeft(70));
    }
private:
    DeviceChange change_;
    juce::TextButton open_, ignore_;
};

}
