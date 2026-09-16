// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/Colours.h"
#include "hum/dsp/BeatGrid.h"
#include "gui/editor/BrickBindings.h"
#include "gui/host/BrickHost.h"
#include "gui/host/EngineHostDeck.h"
#include "gui/style/LookAndFeel.h"
#include "gui/common/Localisation.h"

namespace hum {

class DeckView : public juce::Component,
                 public juce::FileDragAndDropTarget,
                 private juce::Timer {
public:
    static constexpr int    kOverviewH = 34;
    static constexpr double kViewSeconds = 6.0;

    DeckView(BrickHost& host, std::string organism, const Bindings& bound)
        : host_(host), name_(std::move(organism)),
          bpm_(bound(bind::kBpm)), cue_(bound(bind::kCue)), file_(bound(bind::kFile)),
          gridOffset_(bound(bind::kGridOffset)), hotCuePrefix_(bound(bind::kHotCuePrefix)),
          hq_(bound(bind::kHq)), key_(bound(bind::kKey)), loop_(bound(bind::kLoop)),
          loopIn_(bound(bind::kLoopIn)), loopOut_(bound(bind::kLoopOut)), quantize_(bound(bind::kQuantize)),
          grid_(tr("deck.set-grid", "Set Grid")), x2_("x2"), half_("/2") {
        for (auto* b : { &grid_, &x2_, &half_ }) {
            b->setColour(juce::TextButton::buttonColourId, Palette::panelLight);
            b->setColour(juce::TextButton::textColourOffId, Palette::text);
            addAndMakeVisible(*b);
        }
        grid_.setTooltip(tr("deck.anchor-the-beatgrid-beat-1", "Anchor the beatgrid (beat 1) at the playhead"));
        x2_.setTooltip(tr("deck.double-the-detected-bpm-grid", "Double the detected BPM (grid x2)"));
        half_.setTooltip(juce::String::fromUTF8("Halve the detected BPM (grid \xc3\xb7""2)"));
        grid_.onClick = [this] { host_.setParam(name_, gridOffset_, (double) host_.decks().position(name_)); repaint(); };
        x2_.onClick   = [this] { scaleBpm(2.0); };
        half_.onClick = [this] { scaleBpm(0.5); };
        startTimerHz(20);
    }

    void resized() override {
        auto r = getLocalBounds();
        overview_ = r.removeFromTop(kOverviewH);
        main_ = r;
        grid_.setBounds(getWidth() - 150, 2, 56, 18);
        x2_.setBounds(getWidth() - 60, 2, 28, 18);
        half_.setBounds(getWidth() - 30, 2, 28, 18);
    }

    void paint(juce::Graphics& g) override {
        g.setColour(Palette::panel.darker(0.4f));
        g.fillRect(getLocalBounds());

        const int64_t len = host_.decks().length(name_);
        const double fileSr = host_.decks().sampleRate(name_);
        const auto peaks = host_.decks().waveform(name_);
        const int64_t pos = host_.decks().position(name_);

        if (peaks.empty() || len <= 0) {
            g.setColour(Palette::textDim);
            g.setFont(juce::FontOptions(12.0f));
            g.drawText(tr("deck.drop-a-track-here", "Drop a track here"), getLocalBounds(), juce::Justification::centred);
            return;
        }

        const double span = juce::jmax(1.0, kViewSeconds * fileSr);
        const double left = (double) pos - span * 0.5;
        BeatGrid grid{juce::jmax(1.0, host_.liveParamValue(name_, bpm_)),
                      (int64_t) host_.liveParamValue(name_, gridOffset_)};

        paintWave(g, overview_, peaks, 0.0, (double) len, len, Palette::accent.withAlpha(alpha::mid));
        auto ovX = [&](double s) { return overview_.getX() + (float) (s / (double) len) * overview_.getWidth(); };
        g.setColour(Palette::text.withAlpha(alpha::veil));
        g.fillRect(juce::Rectangle<float>(ovX(left), (float) overview_.getY(),
                                          ovX(left + span) - ovX(left), (float) overview_.getHeight()));
        g.setColour(juce::Colours::white);
        g.drawVerticalLine((int) ovX((double) pos), (float) overview_.getY(), (float) overview_.getBottom());

        g.setColour(Palette::panel.darker(0.6f));
        g.fillRect(main_);
        paintWave(g, main_, peaks, left, span, len, Palette::accent);
        auto mnX = [&](double s) { return main_.getX() + (float) ((s - left) / span) * main_.getWidth(); };

        if (host_.liveParamValue(name_, loop_) >= 0.5) {
            const double li = host_.liveParamValue(name_, loopIn_), lo = host_.liveParamValue(name_, loopOut_);
            if (lo > li) { g.setColour(Palette::accent.withAlpha(alpha::mist));
                g.fillRect(juce::Rectangle<float>(mnX(li), (float) main_.getY(), mnX(lo) - mnX(li), (float) main_.getHeight())); }
        }
        const double spb = grid.samplesPerBeat(fileSr);
        if (spb > 1.0) {
            double b0 = std::floor(grid.sampleToBeat(left, fileSr));
            for (double bk = b0; ; bk += 1.0) {
                const double s = grid.beatToSample(bk, fileSr);
                if (s > left + span) break;
                if (s < 0 || s > (double) len) continue;
                const bool down = std::fmod(std::fabs(bk), 4.0) < 0.001;
                g.setColour(down ? Palette::text.withAlpha(alpha::mid) : Palette::border.withAlpha(alpha::dim));
                g.drawVerticalLine((int) mnX(s), (float) main_.getY(), (float) main_.getBottom());
            }
        }
        auto inView = [&](double s) { return s >= left && s <= left + span && s >= 0 && s <= (double) len; };
        const double cue = host_.liveParamValue(name_, cue_);
        if (cue > 0 && inView(cue)) {
            g.setColour(juce::Colours::orange);
            g.drawVerticalLine((int) mnX(cue), (float) main_.getY(), (float) main_.getBottom());
        }
        g.setFont(juce::FontOptions(9.0f));
        for (int i = 1; i <= 8; ++i) {
            const double h = host_.liveParamValue(name_, hotCuePrefix_ + std::to_string(i));
            if (h <= 0.0 || !inView(h)) continue;
            const float x = mnX(h);
            g.setColour(Palette::accent);
            g.drawVerticalLine((int) x, (float) main_.getY(), (float) main_.getBottom());
            g.drawText(juce::String(i), (int) x + 1, main_.getY() + 1, 9, 10, juce::Justification::topLeft);
        }

        g.setColour(juce::Colours::white);
        g.drawVerticalLine(main_.getCentreX(), (float) main_.getY(), (float) main_.getBottom());

        const double eff = host_.decks().effectiveBpm(name_);
        g.setColour(Palette::accent);
        g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        if (eff > 0.0) g.drawText(juce::String(eff, 1) + tr("deck.bpm", " BPM"), main_.reduced(6, 4), juce::Justification::topLeft);
        juce::String info = host_.liveParamText(name_, key_);
        if (host_.liveParamValue(name_, quantize_) >= 0.5) info += "  Q";
        if (host_.liveParamValue(name_, hq_) >= 0.5) info += "  HQ";
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(11.0f));
        g.drawText(info, main_.reduced(6, 4), juce::Justification::topRight);
    }

    bool isInterestedInFileDrag(const juce::StringArray& files) override {
        for (auto& f : files) if (hasAudioExt(f)) return true;
        return false;
    }
    void filesDropped(const juce::StringArray& files, int, int) override {
        for (auto& f : files) if (hasAudioExt(f)) { host_.setParamText(name_, file_, f.toStdString()); break; }
        repaint();
    }

    void mouseDown(const juce::MouseEvent& e) override {
        if (overview_.contains(e.getPosition())) { drag_ = Drag::Overview; scrubOverview(e.x); return; }
        if (e.mods.isAltDown()) {
            host_.setParam(name_, gridOffset_, (double) mainXToSample(e.x)); drag_ = Drag::None; repaint(); return;
        }
        if (e.mods.isShiftDown()) { drag_ = Drag::Loop; loopAnchor_ = mainXToSample(e.x); return; }
        drag_ = Drag::Scrub;
        dragStartSample_ = host_.decks().position(name_);
        dragStartX_ = e.x;
        host_.ensureAudio();
        host_.decks().setScrub(name_, true, (double) dragStartSample_);
    }
    void mouseDrag(const juce::MouseEvent& e) override {
        if (drag_ == Drag::Overview) { scrubOverview(e.x); return; }
        if (drag_ == Drag::Loop) {
            const int64_t a = loopAnchor_, b = mainXToSample(e.x);
            host_.setParam(name_, loopIn_, (double) std::min(a, b));
            host_.setParam(name_, loopOut_, (double) std::max(a, b));
            repaint(); return;
        }
        if (drag_ == Drag::Scrub) {
            const double fileSr = host_.decks().sampleRate(name_);
            const double spp = juce::jmax(1.0, kViewSeconds * fileSr) / juce::jmax(1, main_.getWidth());
            const int64_t len = host_.decks().length(name_);
            const double np = (double) dragStartSample_ - (double) (e.x - dragStartX_) * spp;
            host_.decks().setScrub(name_, true, juce::jlimit(0.0, (double) len, np));
        }
    }
    void mouseUp(const juce::MouseEvent&) override {
        if (drag_ == Drag::Scrub) host_.decks().setScrub(name_, false, 0.0);
        drag_ = Drag::None;
    }

private:
    void timerCallback() override { repaint(); }

    static bool hasAudioExt(const juce::String& f) {
        auto l = f.toLowerCase();
        return l.endsWith(".wav") || l.endsWith(".aiff") || l.endsWith(".aif") ||
               l.endsWith(".flac") || l.endsWith(".mp3") || l.endsWith(".ogg");
    }
    void scaleBpm(double k) {
        const double b = host_.liveParamValue(name_, bpm_) * k;
        host_.setParam(name_, bpm_, juce::jlimit(20.0, 300.0, b));
        repaint();
    }
    void paintWave(juce::Graphics& g, juce::Rectangle<int> r, const std::vector<float>& peaks,
                   double startSample, double spanSamples, int64_t len, juce::Colour col) {
        g.setColour(col);
        const float midY = (float) r.getCentreY();
        for (int x = 0; x < r.getWidth(); ++x) {
            const double s = startSample + (double) x / r.getWidth() * spanSamples;
            if (s < 0 || s >= (double) len) continue;
            const size_t b = (size_t) (s / (double) len * peaks.size());
            const float pk = peaks[std::min(b, peaks.size() - 1)];
            const float hh = pk * (r.getHeight() * 0.46f);
            g.drawVerticalLine(r.getX() + x, midY - hh, midY + hh);
        }
    }
    int64_t mainXToSample(int x) const {
        const int64_t len = host_.decks().length(name_);
        const double fileSr = host_.decks().sampleRate(name_);
        const double span = juce::jmax(1.0, kViewSeconds * fileSr);
        const double left = (double) host_.decks().position(name_) - span * 0.5;
        const double s = left + (double) (x - main_.getX()) / juce::jmax(1, main_.getWidth()) * span;
        return (int64_t) juce::jlimit(0.0, (double) len, s);
    }
    void scrubOverview(int x) {
        const int64_t len = host_.decks().length(name_);
        const double frac = juce::jlimit(0.0, 1.0, (double) (x - overview_.getX()) / juce::jmax(1, overview_.getWidth()));
        host_.decks().seek(name_, (int64_t) (frac * (double) len));
    }

    BrickHost& host_;
    std::string name_;
    std::string bpm_, cue_, file_, gridOffset_, hotCuePrefix_, hq_, key_, loop_, loopIn_, loopOut_, quantize_;
    juce::TextButton grid_, x2_, half_;
    juce::Rectangle<int> overview_, main_;
    enum class Drag { None, Overview, Loop, Scrub } drag_ = Drag::None;
    int64_t loopAnchor_ = 0, dragStartSample_ = 0;
    int dragStartX_ = 0;
};

}
