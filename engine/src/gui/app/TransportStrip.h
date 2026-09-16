// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/host/EngineHost.h"
#include "gui/style/IconButton.h"
#include "gui/style/LookAndFeel.h"
#include "gui/app/TempoSlider.h"
#include "gui/app/ToolbarLadder.h"
#include "gui/app/TransportWidgets.h"
#include "gui/common/UiTicker.h"
#include "gui/common/Localisation.h"

namespace hum {

class TransportStrip : public juce::Component {
public:
    static constexpr int kHeight = 30;

    explicit TransportStrip(EngineHost& host) : host_(host) {
        play_.setLead(true);
        for (auto* b : {&fromStart_, &play_, &stop_, &record_, &loop_}) addAndMakeVisible(*b);
        fromStart_.onClick = [this] { if (onPlayFromStart) onPlayFromStart(); };
        play_.onClick      = [this] { if (onPlay) onPlay(); };
        stop_.onClick      = [this] { if (onStop) onStop(); };
        record_.onClick    = [this] { if (onRecord) onRecord(); };
        loop_.onClick      = [this] { if (onLoop) onLoop(); };

        addAndMakeVisible(tempo_);
        tempo_.setSliderStyle(juce::Slider::IncDecButtons);
        tempo_.setRange(kTempoMin, kTempoMax, 0.1);
        tempo_.setTextValueSuffix(" BPM");
        tempo_.setTextBoxStyle(juce::Slider::TextBoxLeft, false, kTempoTextW, 22);
        tempo_.setValue(host_.liveTempo(), juce::dontSendNotification);
        tempo_.onValueChange = [this] { host_.performTempo(tempo_.getValue()); };
        tempo_.onPopup = [this](juce::Point<int> at) { if (onTempoMenu) onTempoMenu(at); };

        addAndMakeVisible(tsig_);
        tsig_.get = [this] { return host_.automation().meterAt(host_.positionBeats()); };
        tsig_.set = [this](Meter m) { host_.automation().setTimeSignature(m.beats, m.unit); };
        tsig_.automated = [this] { return host_.automation().meterAutomated(); };
        tsig_.setAutomated = [this](bool on) { host_.automation().setMeterAutomated(on); };

        addAndMakeVisible(clock_);
        tickId_ = UiTicker::instance().add([this] { tick(); });
    }
    ~TransportStrip() override { UiTicker::instance().remove(tickId_); }

    std::function<void()> onPlay, onPlayFromStart, onStop, onRecord, onLoop;
    std::function<void(juce::Point<int>)> onTempoMenu;

    void paint(juce::Graphics& g) override {
        g.setColour(Palette::panel);
        g.fillAll();
        g.setColour(Palette::border);
        g.drawHorizontalLine(getHeight() - 1, 0.0f, (float) getWidth());
    }

    static constexpr int kTempoTextW = TempoSlider::kNumberW;

    void resized() override {
        auto r = getLocalBounds().reduced(4, 3);
        auto put = [&r](juce::Component& c, int w) {
            c.setBounds(r.removeFromLeft(w).reduced(1, 0));
        };
        put(fromStart_, 26);
        put(play_, 34);
        put(stop_, 26);
        put(record_, 28);
        put(loop_, 26);
        r.removeFromLeft(10);
        constexpr int kClockW = 110;
        if (r.getWidth() >= toolbar::kTempoW + toolbar::kTsigW + kClockW)
            clock_.setBounds(r.removeFromRight(kClockW));
        else
            clock_.setBounds({});
        put(tempo_, juce::jmin(r.getWidth(), toolbar::kTempoW));
        if (r.getWidth() >= toolbar::kTsigW) put(tsig_, toolbar::kTsigW);
        else tsig_.setBounds({});
    }

private:
    void tick() {
        if (!isShowing()) return;
        play_.setOn(host_.isPlaying());
        loop_.setOn(host_.automation().loopEnabled());
        const bool preRoll = host_.record().preRolling();
        const bool armed = host_.record().armed() || preRoll;
        const bool pending = preRoll || (armed && !host_.isPlaying());
        record_.setOn(armed && (!pending || (++blink_ / 15) % 2 == 0));
        clock_.setPosition(host_.positionBar(), host_.positionBeat(), host_.positionSeconds());
        tsig_.repaint();
        if (const double bpm = host_.liveTempo();
            !tempo_.userDragging() && std::abs(tempo_.getValue() - bpm) > 0.01)
            tempo_.setValue(bpm, juce::dontSendNotification);
    }

    EngineHost& host_;
    IconButton fromStart_{IconButton::Glyph::PlayFromStart, tr("transport-strip.play-from-start", "Play From Start")};
    IconButton play_{IconButton::Glyph::Play, tr("transport-strip.play-space", "Play (Space)")};
    IconButton stop_{IconButton::Glyph::Stop, "Stop"};
    IconButton record_{IconButton::Glyph::Record, tr("transport-strip.record-the-performance", "Record the performance")};
    IconButton loop_{IconButton::Glyph::Loop, tr("transport-strip.enable-automation-loop", "Enable Automation Loop")};
    TempoSlider tempo_;
    TimeSigChip tsig_;
    ClockReadout clock_;
    int tickId_ = 0;
    int blink_ = 0;
};

}
