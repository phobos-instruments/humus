// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <memory>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/BrickBindings.h"
#include "gui/style/Colours.h"
#include "gui/host/BrickHost.h"
#include "gui/style/IconGlyph.h"
#include "gui/style/LookAndFeel.h"
#include "gui/bricks/PolledBrick.h"
#include "gui/video/VideoDeckPool.h"
#include "gui/editor/juce/JuceVideoTarget.h"

namespace hum {

class TransportGlyphButton : public juce::Button {
public:
    explicit TransportGlyphButton(IconGlyph glyph)
        : juce::Button(juce::String(kIconGlyphNames[(size_t) glyph])),
          glyph_(glyph) {}

    void paintButton(juce::Graphics& g, bool over, bool down) override {
        auto r = getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(Palette::panel.brighter(down ? 0.20f : over ? 0.12f : 0.05f));
        g.fillRoundedRectangle(r, 4.0f);
        g.setColour(getToggleState() ? Palette::accent : Palette::border);
        g.drawRoundedRectangle(r.reduced(0.5f), 4.0f, 1.0f);
        const auto tint = !isEnabled() ? Palette::textDim.withAlpha(alpha::dim)
                          : getToggleState() ? Palette::accent
                                             : Palette::text;
        drawIconGlyph(g, glyph_, r.reduced(r.getHeight() * 0.28f), tint, isEnabled(),
                      getToggleState());
    }

private:
    IconGlyph glyph_;
};

class VideoTransportBrick : public PolledBrick {
public:
    VideoTransportBrick(BrickHost& host, std::string organism, const Bindings& bound)
        : PolledBrick(host, std::move(organism)), fileParam_(bound(bind::kFile)) {
        for (auto* b : {&play_, &pause_, &stop_}) addAndMakeVisible(*b);
        addAndMakeVisible(scrub_);
        addAndMakeVisible(time_);
        play_.onClick = [this] {
            JuceVideoTarget t(layer());
            video::VideoTransportModel::play(t.get());
        };
        pause_.onClick = [this] {
            JuceVideoTarget t(layer());
            video::VideoTransportModel::pause(t.get());
        };
        stop_.onClick = [this] {
            JuceVideoTarget t(layer());
            video::VideoTransportModel::stop(t.get());
        };
        scrub_.setRange(0.0, 1.0, 0.0);
        scrub_.setSliderStyle(juce::Slider::LinearHorizontal);
        scrub_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        scrub_.onDragStart = [this] { scrubbing_ = true; };
        scrub_.onDragEnd = [this] {
            scrubbing_ = false;
            seekToScrub();
        };
        scrub_.onValueChange = [this] {
            if (scrubbing_) seekToScrub();
        };
        time_.setJustificationType(juce::Justification::centredRight);
        time_.setFont(juce::FontOptions(11.0f));
        time_.setColour(juce::Label::textColourId, Palette::textDim);
        time_.setInterceptsMouseClicks(false, false);
        ensureDeck();
    }

    void reloadValues() override {}
    void refreshAutomatedValues() override {}
    void refreshNow() { poll(); }
    bool playLit() const { return play_.getToggleState(); }
    bool pauseLit() const { return pause_.getToggleState(); }
    int preferredContentWidth() const override { return 269; }
    int preferredContentHeight(int) const override { return 52; }

    void resized() override {
        auto r = getLocalBounds();
        auto row = r.removeFromTop(22);
        play_.setBounds(row.removeFromLeft(34));
        row.removeFromLeft(4);
        pause_.setBounds(row.removeFromLeft(34));
        row.removeFromLeft(4);
        stop_.setBounds(row.removeFromLeft(34));
        time_.setBounds(row);
        r.removeFromTop(4);
        scrub_.setBounds(r.removeFromTop(24));
    }

private:
    std::string fileParam_;
    std::shared_ptr<VideoLayer> layer() const {
        return VideoDeckPool::instance().peek(name_);
    }

    void ensureDeck() {
        const auto path = juce::String(host_.liveParamText(name_, fileParam_));
        const auto file = VideoDeckPool::resolveTape(host_.documentPath(), path);
        if (file == juce::File()) {
            held_.reset();
            heldPath_ = {};
            return;
        }
        if (held_ == nullptr || heldPath_ != file.getFullPathName()) {
            heldPath_ = file.getFullPathName();
            held_ = VideoDeckPool::instance().open(name_, heldPath_);
        }
    }

    void seekToScrub() {
        JuceVideoTarget t(layer());
        video::VideoTransportModel::seekFraction(t.get(), scrub_.getValue());
    }

    void poll() override {
        ensureDeck();
        JuceVideoTarget t(layer());
        const auto shown = video::VideoTransportModel::shown(t.get());
        const bool live = shown.live;
        for (auto* b : {&play_, &pause_, &stop_}) b->setEnabled(live);
        scrub_.setEnabled(live);
        if (!live) {
            wasPaused_ = -1;
            time_.setText("", juce::dontSendNotification);
            return;
        }
        if (!scrubbing_)
            scrub_.setValue(shown.fraction, juce::dontSendNotification);
        time_.setText(juce::String(shown.time), juce::dontSendNotification);
        const int paused = shown.paused;
        if (paused != wasPaused_) {
            wasPaused_ = paused;
            play_.setToggleState(paused == 0, juce::dontSendNotification);
            pause_.setToggleState(paused == 1, juce::dontSendNotification);
        }
    }

    TransportGlyphButton play_{IconGlyph::Play}, pause_{IconGlyph::Pause},
        stop_{IconGlyph::Stop};
    juce::Slider scrub_;
    juce::Label time_;
    std::shared_ptr<VideoLayer> held_;
    juce::String heldPath_;
    bool scrubbing_ = false;
    int wasPaused_ = -1;
};

}
