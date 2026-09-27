// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/AutomateMenu.h"
#include "gui/editor/BrickBindings.h"
#include "gui/editor/Mappable.h"
#include "gui/editor/ParamRanges.h"
#include "gui/style/Colours.h"
#include "gui/host/BrickHost.h"
#include "gui/host/EngineHostFiles.h"
#include "gui/style/IconGlyph.h"
#include "gui/style/LookAndFeel.h"
#include "gui/common/Localisation.h"
#include "gui/bricks/SegmentClock.h"
#include "gui/editor/files/TransportModel.h"

namespace hum {

class TransportButton : public juce::Button {
public:
    explicit TransportButton(IconGlyph g) : juce::Button({}), glyph_(g) {
        setTriggeredOnMouseDown(true);
    }

    void setGlyph(IconGlyph g) { if (g != glyph_) { glyph_ = g; repaint(); } }
    void setOn(bool on) { if (on != on_) { on_ = on; repaint(); } }

    void paintButton(juce::Graphics& g, bool over, bool down) override {
        auto r = getLocalBounds().toFloat().reduced(1.5f);
        g.setColour(on_ ? Palette::accent.withAlpha(alpha::muted)
                        : down ? Palette::accent.darker(0.2f)
                               : over ? Palette::panelLight.brighter(0.1f)
                                      : Palette::panelLight);
        g.fillRoundedRectangle(r, 4.0f);
        g.setColour(on_ ? Palette::accent : Palette::border);
        g.drawRoundedRectangle(r, 4.0f, 1.0f);

        const auto ink = !isEnabled() ? Palette::textDim : on_ ? Palette::accent : Palette::text;
        drawIconGlyph(g, glyph_, r.reduced(r.getWidth() * 0.24f, r.getHeight() * 0.24f),
                      ink, isEnabled(), on_);
    }

private:
    IconGlyph glyph_ = IconGlyph::Play;
    bool on_ = false;
};

inline constexpr int kPlateHeight = 34;
inline constexpr int kTwoRowHeight = 52;
inline constexpr int kSliderHeight = 24;
inline constexpr int kShortButton = 24;
inline constexpr int kTallButton = 36;
inline constexpr int kNudgeRoom = 260;
inline constexpr int kNarrowClock = 116;

using TransportKey = Mappable<TransportButton>;

class FileTransport : public juce::Component, private juce::Timer {
public:
    static constexpr unsigned kBlinkMs = 400;
    FileTransport(BrickHost& host, std::string organism, const Bindings& bound, bool withSeekBar = true)
        : withSeekBar_(withSeekBar), host_(host), name_(organism),
          transport_(host, host.files(), organism, bound(bind::kActive), bound(bind::kLoop),
                     bound(bind::kRecord)),
          rewind_(IconGlyph::GoToStart),
          play_(IconGlyph::Play),
          stop_(IconGlyph::Stop),
          loop_(IconGlyph::Loop),
          back_(IconGlyph::ScanBack),
          forward_(IconGlyph::ScanForward),
          record_(IconGlyph::Record) {
        rewind_.onClick = [this] { transport_.rewind(); };
        play_.onClick   = [this] { transport_.togglePlay(); refreshStates(); };
        stop_.onClick   = [this] { transport_.stop(); refreshStates(); };
        loop_.onClick   = [this] { transport_.toggleLoop(); refreshStates(); };
        rewind_.setTooltip(tr("file-transport.rewind-to-start", "Rewind to start"));
        play_.setTooltip(tr("file-transport.play-pause", "Play / Pause"));
        stop_.setTooltip(tr("file-transport.stop-rewind-to-start", "Stop (rewind to start)"));
        loop_.setTooltip(tr("file-transport.loop", "Loop"));
        back_.onClick = [this] { transport_.nudgeSeconds(-files::kNudgeSeconds); };
        forward_.onClick = [this] { transport_.nudgeSeconds(files::kNudgeSeconds); };
        back_.setTooltip(tr("file-transport.back-ten", "Back ten seconds, hold to run back faster"));
        forward_.setTooltip(tr("file-transport.forward-ten",
                               "Forward ten seconds, hold to run on faster"));
        record_.onClick = [this] { transport_.toggleArmed(); refreshStates(); };
        record_.setTooltip(tr("file-transport.arm-to-record", "Arm to record"));
        learnable(rewind_, bound(bind::kRewind));
        learnable(back_, bound(bind::kSeekBack));
        learnable(play_, bound(bind::kActive));
        learnable(stop_, bound(bind::kStop));
        learnable(forward_, bound(bind::kSeekForward));
        learnable(loop_, bound(bind::kLoop));
        learnable(record_, bound(bind::kRecord));
        for (auto* b : { &rewind_, &back_, &play_, &stop_, &forward_, &loop_ })
            addAndMakeVisible(*b);
        if (transport_.arms()) addAndMakeVisible(record_);

        slider_.setSliderStyle(juce::Slider::LinearHorizontal);
        slider_.setRange(0.0, 1.0, 0.0);
        slider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider_.onDragStart = [this] { scrubbing_ = true; };
        slider_.onValueChange = [this] { if (scrubbing_) commitSeek(); };
        slider_.onDragEnd = [this] { commitSeek(); scrubbing_ = false; };
        if (withSeekBar_) addAndMakeVisible(slider_);

        addAndMakeVisible(clock_);

        update();
        startTimerHz(20);
    }

    void paint(juce::Graphics& g) override {
        if (getHeight() < kPlateHeight) return;
        auto r = getLocalBounds().toFloat();
        g.setColour(Palette::panel.darker(0.35f));
        g.fillRoundedRectangle(r, 5.0f);
        g.setColour(Palette::border);
        g.drawRoundedRectangle(r.reduced(0.5f), 5.0f, 1.0f);
    }

    void resized() override {
        auto r = getLocalBounds();
        if (withSeekBar_ && r.getHeight() >= kTwoRowHeight) {
            slider_.setBounds(r.removeFromBottom(kSliderHeight));
            r.removeFromBottom(4);
            layoutRow(r);
            return;
        }
        if (withSeekBar_) {
            const int bh = juce::jmin(r.getHeight(), kShortButton);
            auto row = r.withSizeKeepingCentre(r.getWidth(), bh);
            int unit = 0;
            const bool room = r.getWidth() >= kNudgeRoom + kNarrowClock;
            buttonWidths(bh, room, unit);
            layoutButtons(row, unit, room);
            clock_.setBounds(row.removeFromRight(juce::jmin(row.getWidth(), kNarrowClock)));
            row.removeFromRight(4);
            slider_.setBounds(row);
            return;
        }
        layoutRow(r);
    }

private:
    void learnable(TransportKey& b, const std::string& param) {
        if (param.empty()) return;
        b.onRightClick = [this, param](juce::Point<int> at) {
            showAutomateMenu(host_, name_, param, at, [this] { update(); repaint(); });
        };
        learned_.push_back({&b, param});
    }

    struct Learned { TransportKey* button; std::string param; };

    int buttonWidths(int bh, bool room, int& unit) const {
        unit = juce::jmax(kShortButton, bh);
        return unit * (4 + (room ? 2 : 0) + (transport_.arms() ? 1 : 0));
    }

    void layoutRow(juce::Rectangle<int> r) {
        auto full = r.reduced(4, 3);
        const int bh = juce::jlimit(kShortButton, kTallButton, full.getHeight());
        const bool room = full.getWidth() >= kNudgeRoom;
        int unit = 0;
        const int needed = buttonWidths(bh, room, unit);
        clock_.setBounds(full.removeFromRight(std::max(0, full.getWidth() - needed - 8)));
        auto row = full.withSizeKeepingCentre(full.getWidth(), juce::jmin(full.getHeight(), bh));
        layoutButtons(row, unit, room);
    }

    void layoutButtons(juce::Rectangle<int>& row, int unit, bool room) {
        back_.setVisible(room);
        forward_.setVisible(room);
        rewind_.setBounds(row.removeFromLeft(unit));
        if (room) back_.setBounds(row.removeFromLeft(unit));
        play_.setBounds(row.removeFromLeft(unit));
        stop_.setBounds(row.removeFromLeft(unit));
        if (room) forward_.setBounds(row.removeFromLeft(unit));
        loop_.setBounds(row.removeFromLeft(unit));
        if (transport_.arms()) record_.setBounds(row.removeFromLeft(unit));
    }

    void timerCallback() override {
        update();
        holdNudge();
        for (const auto& [button, param] : learned_) {
            button->setMarks(paramIsControlled(host_, name_, param), host_.rollLocked(name_, param));
            button->setLit(host_.firedRecently(name_, param));
        }
    }

    void holdNudge() {
        const int direction = back_.isDown() ? -1 : forward_.isDown() ? 1 : 0;
        if (direction == 0) {
            heldTicks_ = 0;
            return;
        }
        ++heldTicks_;
        const double step = files::nudgeHoldStep(heldTicks_);
        if (step > 0.0) transport_.nudgeSeconds(direction * step);
    }

    static bool blinkOn() {
        return (juce::Time::getMillisecondCounter() / kBlinkMs) % 2 == 0;
    }

    void refreshStates() {
        const bool rolling = transport_.active();
        const bool held = transport_.armed() && !rolling;
        play_.setGlyph(rolling ? IconGlyph::Pause : IconGlyph::Play);
        play_.setOn(held && blinkOn());
        loop_.setOn(transport_.looping());
        record_.setOn(transport_.armed());
    }

    void commitSeek() { transport_.seekTo(slider_.getValue()); }

    void update() {
        if (!scrubbing_ && withSeekBar_)
            slider_.setValue(transport_.fraction(), juce::dontSendNotification);
        clock_.setTime(transport_.positionSeconds(), transport_.lengthSeconds());
        refreshStates();
    }

    const bool withSeekBar_;
    BrickHost& host_;
    std::string name_;
    files::TransportModel transport_;
    TransportKey rewind_, play_, stop_, loop_, back_, forward_, record_;
    juce::Slider slider_;
    SegmentClock clock_;
    bool scrubbing_ = false;
    int heldTicks_ = 0;
    std::vector<Learned> learned_;
};

}
