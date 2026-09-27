// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <functional>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/browser/BrowserEntry.h"
#include "gui/browser/Audition.h"
#include "gui/browser/BrowserPaint.h"
#include "gui/common/Localisation.h"

namespace hum::browser {

class AuditionStrip : public juce::Component {
public:
    static constexpr int kHeight = 56, kPad = 10, kPlayW = 32, kControlsW = 250;

    explicit AuditionStrip(Audition& audition) : audition_(audition) {
        for (auto* b : {&loop_, &sync_}) {
            b->setClickingTogglesState(true);
            addAndMakeVisible(*b);
        }
        loop_.setButtonText(tr("browser.loop", "Loop"));
        sync_.setButtonText(tr("browser.sync", "Sync"));
        sync_.setTooltip(tr("browser.sync-tip", "Play loops at the patch tempo"));
        power_.setButtonText(tr("browser.turn-on-audio", "Turn on audio"));
        power_.onClick = [this] {
            if (turnEngineOn) turnEngineOn();
        };
        addChildComponent(power_);
        loop_.onClick = [this] { audition_.setLoop(loop_.getToggleState()); };
        sync_.onClick = [this] { audition_.setSync(sync_.getToggleState()); };
        volume_.setSliderStyle(juce::Slider::LinearBar);
        volume_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        volume_.setRange(0.0, 1.5, 0.0);
        volume_.setValue(audition_.gain(), juce::dontSendNotification);
        volume_.setTooltip(tr("browser.volume", "Preview volume"));
        volume_.onValueChange = [this] { audition_.setGain((float) volume_.getValue()); };
        addAndMakeVisible(volume_);
        loop_.setToggleState(audition_.loop(), juce::dontSendNotification);
        sync_.setToggleState(audition_.sync(), juce::dontSendNotification);
        restyle();
    }

    void restyle() {
        for (auto* b : {&loop_, &sync_}) {
            b->setColour(juce::TextButton::buttonColourId, Palette::panelLight);
            b->setColour(juce::TextButton::buttonOnColourId, Palette::accent);
            b->setColour(juce::TextButton::textColourOnId, paint::onAccent());
            b->setColour(juce::TextButton::textColourOffId, Palette::text);
        }
        volume_.setColour(juce::Slider::trackColourId, Palette::textDim.withAlpha(alpha::mid));
        volume_.setColour(juce::Slider::backgroundColourId, Palette::panelLight);
        power_.setColour(juce::TextButton::buttonColourId, Palette::accent);
        power_.setColour(juce::TextButton::textColourOffId, paint::onAccent());
        repaint();
    }
    void lookAndFeelChanged() override { restyle(); }

    std::function<bool()> engineRunning;
    std::function<void()> turnEngineOn;

    bool engineOff() const { return engineRunning && !engineRunning(); }
    bool offersPower() const { return power_.isVisible(); }
    void pressPower() { power_.onClick(); }

    void show(const Entry& entry, const juce::String& hint) {
        entry_ = entry;
        hint_ = hint;
        repaint();
    }

    void tick() {
        if (const bool off = engineOff(); off != power_.isVisible()) {
            power_.setVisible(off);
            if (!off && audition_.current() == entry_.path) audition_.play(entry_.path, entry_.facts.bpm);
            resized();
            repaint();
        }
        const bool playing = audition_.playing();
        const double pos = audition_.position();
        if (playing != wasPlaying_ || std::abs(pos - lastPos_) > 0.01) repaint(wave().toNearestInt().expanded(2).getUnion(playBounds().toNearestInt()));
        wasPlaying_ = playing;
        lastPos_ = pos;
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(Palette::panel);
        g.setColour(Palette::border.withAlpha(alpha::muted));
        g.drawHorizontalLine(0, 0.0f, (float) getWidth());
        paintPlay(g);
        const auto w = wave();
        g.setColour(Palette::background);
        g.fillRoundedRectangle(w, 4.0f);
        const bool mine = audition_.current() == entry_.path;
        const double length = entry_.facts.seconds > 0.0 ? entry_.facts.seconds : audition_.length();
        const float played = mine && audition_.playing() && length > 0.0 ? (float) (audition_.position() / length) : 0.0f;
        paint::drawPeaks(g, entry_.facts.peaks, w.reduced(4.0f, 5.0f), Palette::textDim.withAlpha(alpha::strong));
        if (played > 0.0f) {
            juce::Graphics::ScopedSaveState keep(g);
            g.reduceClipRegion(w.withWidth(w.getWidth() * juce::jlimit(0.0f, 1.0f, played)).toNearestInt());
            paint::drawPeaks(g, entry_.facts.peaks, w.reduced(4.0f, 5.0f), Palette::accent);
        }
        if (played > 0.0f) {
            g.setColour(Palette::accent);
            g.fillRect(w.getX() + w.getWidth() * played, w.getY(), 1.5f, w.getHeight());
        }
        auto info = getLocalBounds().withTrimmedLeft((int) w.getRight() + kPad).withTrimmedRight(kControlsW).reduced(0, 8);
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(12.5f));
        g.drawText(juce::String::fromUTF8(fileName(entry_.path).c_str()), info.removeFromTop(info.getHeight() / 2),
                   juce::Justification::bottomLeft, true);
        g.setColour(power_.isVisible() ? Palette::warnAmber() : Palette::textDim);
        g.setFont(juce::FontOptions(11.0f));
        g.drawText(power_.isVisible() ? tr("browser.audio-off", "Audio is off, so previews are silent") : hint_, info,
                   juce::Justification::topLeft, true);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(kPad, 14).removeFromRight(kControlsW - kPad);
        power_.setBounds(r.withSizeKeepingCentre(r.getWidth(), 26));
        loop_.setBounds(r.removeFromLeft(54));
        r.removeFromLeft(6);
        sync_.setBounds(r.removeFromLeft(54));
        r.removeFromLeft(10);
        volume_.setBounds(r.withSizeKeepingCentre(r.getWidth(), 8));
    }

    void mouseUp(const juce::MouseEvent& e) override {
        if (power_.isVisible()) return;
        if (playBounds().contains(e.position)) {
            if (audition_.current() == entry_.path) audition_.toggle();
            else audition_.play(entry_.path, entry_.facts.bpm);
            repaint();
            return;
        }
        const auto w = wave();
        if (w.contains(e.position) && entry_.facts.seconds > 0.0) {
            const double at = entry_.facts.seconds * (e.position.x - w.getX()) / w.getWidth();
            if (audition_.current() != entry_.path || !audition_.playing()) audition_.play(entry_.path, entry_.facts.bpm);
            audition_.seek(at);
        }
    }

private:
    juce::Rectangle<float> playBounds() const {
        return {(float) kPad, (float) (kHeight - kPlayW) * 0.5f, (float) kPlayW, (float) kPlayW};
    }
    juce::Rectangle<float> wave() const {
        const float left = playBounds().getRight() + 8.0f;
        const float width = std::max(80.0f, (float) (getWidth() - kControlsW) * 0.55f - left);
        return {left, 8.0f, width, (float) kHeight - 16.0f};
    }

    void paintPlay(juce::Graphics& g) const {
        const auto r = playBounds();
        const bool playing = audition_.playing() && audition_.current() == entry_.path;
        g.setColour(playing ? Palette::accent : Palette::panelLight);
        g.fillEllipse(r);
        g.setColour(playing ? Palette::background : Palette::text);
        const auto c = r.getCentre();
        if (playing) {
            g.fillRect(c.x - 5.0f, c.y - 5.0f, 10.0f, 10.0f);
        } else {
            juce::Path tri;
            tri.addTriangle(c.x - 4.0f, c.y - 6.0f, c.x - 4.0f, c.y + 6.0f, c.x + 7.0f, c.y);
            g.fillPath(tri);
        }
    }

    Audition& audition_;
    Entry entry_;
    juce::String hint_;
    juce::TextButton loop_, sync_, power_;
    juce::Slider volume_;
    bool wasPlaying_ = false;
    double lastPos_ = 0.0;
};

}
