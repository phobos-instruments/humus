// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <utility>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/bricks/PolledBrick.h"
#include "gui/common/Localisation.h"
#include "gui/host/BrickHost.h"
#include "gui/host/EngineHostFiles.h"
#include "gui/style/Colours.h"
#include "gui/style/LitPad.h"

namespace hum {

class RecorderTransportBrick : public PolledBrick {
public:
    RecorderTransportBrick(BrickHost& host, std::string name, std::string recordParam)
        : PolledBrick(host, std::move(name)), param_(std::move(recordParam)) {
        record_.setTooltip(tr("recorder-transport.arm-or-start-recording",
                              "Arm, or start recording"));
        record_.setClickingTogglesState(false);
        record_.onClick = [this] {
            host_.setParam(name_, param_, asked() ? 0.0 : 1.0);
            poll();
        };
        addAndMakeVisible(record_);

        stop_.setTooltip(tr("recorder-transport.stop-and-close-the-files",
                            "Stop, and close the files"));
        stop_.setClickingTogglesState(false);
        stop_.onClick = [this] {
            host_.setParam(name_, param_, 0.0);
            poll();
        };
        addAndMakeVisible(stop_);
        record_.setLitOverride(Palette::recordRed());
        asked_ = asked();
        rolling_ = host_.files().isRecorderActive(name_);
        apply();
    }

    int preferredContentWidth() const override { return 200; }
    int preferredContentHeight(int) const override { return 26; }

    bool armedForTest() const { return asked() && !rolling_; }
    bool rollingForTest() const { return rolling_; }
    juce::Button& recordForTest() { return record_; }
    juce::Button& stopForTest() { return stop_; }

    void resized() override {
        auto r = getLocalBounds();
        record_.setBounds(r.removeFromLeft(kPadW));
        r.removeFromLeft(6);
        stop_.setBounds(r.removeFromLeft(kPadW));
        r.removeFromLeft(8);
        label_ = r;
    }

    void paint(juce::Graphics& g) override {
        g.setColour(rolling_ ? Palette::recordRed() : Palette::textDim);
        g.setFont(juce::FontOptions(11.0f));
        g.drawText(stateText(), label_, juce::Justification::centredLeft, false);
    }

private:
    static constexpr int kPadW = 34;

    bool asked() const { return host_.liveParamValue(name_, param_) >= 0.5; }

    juce::String stateText() const {
        if (rolling_) return tr("recorder-transport.recording", "recording");
        if (asked()) return tr("recorder-transport.armed", "armed");
        return tr("recorder-transport.idle", "idle");
    }

    void apply() {
        record_.setToggleState(asked_, juce::dontSendNotification);
        record_.setWorking(asked_ && !rolling_);
        stop_.setEnabled(asked_ || rolling_);
        repaint();
    }

    void poll() override {
        const bool rolling = host_.files().isRecorderActive(name_);
        const bool want = asked();
        if (rolling == rolling_ && want == asked_) return;
        rolling_ = rolling;
        asked_ = want;
        apply();
    }

    std::string param_;
    LitPad record_{LitPad::Look{IconGlyph::Record, {}}};
    LitPad stop_{LitPad::Look{IconGlyph::Stop, {}}};
    juce::Rectangle<int> label_;
    bool rolling_ = false, asked_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RecorderTransportBrick)
};

}
