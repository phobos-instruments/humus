#pragma once
#include <cmath>
#include <cstdint>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/EngineHost.h"
#include "gui/IconGlyph.h"
#include "gui/LookAndFeel.h"
#include "gui/Localisation.h"

namespace hum {

class TransportButton : public juce::Button {
public:
    explicit TransportButton(IconGlyph g) : juce::Button({}), glyph_(g) {
        setTriggeredOnMouseDown(true);
    }

    void setGlyph(IconGlyph g) { if (g != glyph_) { glyph_ = g; repaint(); } }
    void setOn(bool on) { if (on != on_) { on_ = on; repaint(); } }

    void paintButton(juce::Graphics& g, bool over, bool down) override {
        auto r = getLocalBounds().toFloat().reduced(2.0f);
        g.setColour(on_ ? Palette::accent.withAlpha(0.35f)
                        : down ? Palette::accent.darker(0.2f)
                               : over ? Palette::panelLight.brighter(0.1f)
                                      : Palette::panelLight);
        g.fillRoundedRectangle(r, 3.0f);
        g.setColour(on_ ? Palette::accent : Palette::border);
        g.drawRoundedRectangle(r, 3.0f, 1.0f);

        drawIconGlyph(g, glyph_, r.reduced(r.getWidth() * 0.24f, r.getHeight() * 0.24f),
                      !isEnabled() ? Palette::textDim : on_ ? Palette::accent : Palette::text,
                      isEnabled());
    }

private:
    IconGlyph glyph_;
    bool on_ = false;
};

class FileTransport : public juce::Component, private juce::Timer {
public:
    FileTransport(EngineHost& host, std::string organism)
        : host_(host), name_(std::move(organism)),
          rewind_(IconGlyph::GoToStart),
          play_(IconGlyph::Play),
          stop_(IconGlyph::Stop),
          loop_(IconGlyph::Loop) {
        rewind_.onClick = [this] { host_.files().seek(name_, 0); };
        play_.onClick   = [this] { setActive(!isActive()); };
        stop_.onClick   = [this] { setActive(false); host_.files().seek(name_, 0); };
        loop_.onClick   = [this] { host_.setParam(name_, "Loop", isLoop() ? 0.0 : 1.0); refreshStates(); };
        rewind_.setTooltip(tr("file-transport.rewind-to-start", "Rewind to start"));
        play_.setTooltip(tr("file-transport.play-pause", "Play / Pause"));
        stop_.setTooltip(tr("file-transport.stop-rewind-to-start", "Stop (rewind to start)"));
        loop_.setTooltip(tr("file-transport.loop", "Loop"));
        for (auto* b : { &rewind_, &play_, &stop_, &loop_ }) addAndMakeVisible(*b);

        slider_.setSliderStyle(juce::Slider::LinearHorizontal);
        slider_.setRange(0.0, 1.0, 0.0);
        slider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider_.onDragStart = [this] { scrubbing_ = true; };
        slider_.onValueChange = [this] { if (scrubbing_) commitSeek(); };
        slider_.onDragEnd = [this] { commitSeek(); scrubbing_ = false; };
        addAndMakeVisible(slider_);

        timeLabel_.setColour(juce::Label::textColourId, Palette::textDim);
        timeLabel_.setFont(juce::FontOptions(11.0f));
        timeLabel_.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(timeLabel_);

        update();
        startTimerHz(20);
    }

    void resized() override {
        auto r = getLocalBounds();
        const int bw = 26;
        if (r.getHeight() >= 44) {
            auto row = r.removeFromTop(24);
            rewind_.setBounds(row.removeFromLeft(bw));
            play_.setBounds(row.removeFromLeft(bw));
            stop_.setBounds(row.removeFromLeft(bw));
            row.removeFromLeft(8);
            loop_.setBounds(row.removeFromLeft(bw));
            timeLabel_.setBounds(row);
            r.removeFromTop(4);
            slider_.setBounds(r.removeFromTop(24));
            return;
        }
        const int bh = juce::jmin(r.getHeight(), 24);
        auto row = r.withSizeKeepingCentre(r.getWidth(), bh);
        rewind_.setBounds(row.removeFromLeft(bw));
        play_.setBounds(row.removeFromLeft(bw));
        stop_.setBounds(row.removeFromLeft(bw));
        row.removeFromLeft(8);
        loop_.setBounds(row.removeFromLeft(bw));
        row.removeFromLeft(8);
        timeLabel_.setBounds(row.removeFromRight(118));
        row.removeFromRight(4);
        slider_.setBounds(row);
    }

private:
    void timerCallback() override { update(); }

    bool isActive() const { return host_.liveParamValue(name_, "Active") >= 0.5; }
    bool isLoop() const { return host_.liveParamValue(name_, "Loop") >= 0.5; }
    void setActive(bool on) {
        if (on) host_.ensureAudio();
        host_.setParam(name_, "Active", on ? 1.0 : 0.0);
        refreshStates();
    }

    void refreshStates() {
        play_.setGlyph(isActive() ? IconGlyph::Pause : IconGlyph::Play);
        loop_.setOn(isLoop());
    }

    void commitSeek() {
        const std::int64_t len = host_.files().playbackLength(name_);
        if (len <= 0) return;
        host_.files().seek(name_, (std::int64_t) (slider_.getValue() * (double) len));
    }

    void update() {
        const std::int64_t len = host_.files().playbackLength(name_);
        const std::int64_t pos = host_.files().playbackPosition(name_);
        const double sr = host_.files().playbackSampleRate(name_);
        if (!scrubbing_)
            slider_.setValue(len > 0 ? juce::jlimit(0.0, 1.0, (double) pos / (double) len) : 0.0,
                             juce::dontSendNotification);
        timeLabel_.setText(formatTime(pos, sr) + " / " + formatTime(len, sr),
                           juce::dontSendNotification);
        refreshStates();
    }

    static juce::String formatTime(std::int64_t samples, double sr) {
        double secs = sr > 0.0 ? (double) samples / sr : 0.0;
        int mins = (int) (secs / 60.0);
        double rem = secs - mins * 60.0;
        return juce::String(mins) + ":" + juce::String(rem, 3).paddedLeft('0', 6);
    }

    EngineHost& host_;
    std::string name_;
    TransportButton rewind_, play_, stop_, loop_;
    juce::Slider slider_;
    juce::Label timeLabel_;
    bool scrubbing_ = false;
};

}
