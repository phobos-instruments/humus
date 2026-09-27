// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/Colours.h"
#include "gui/style/StatusDot.h"
#include "hum/dsp/BeatGrid.h"
#include "gui/editor/BrickBindings.h"
#include "gui/host/BrickHost.h"
#include "gui/editor/decks/DeckModels.h"
#include "gui/video/VideoProbe.h"
#include "gui/host/EngineHostDeck.h"
#include "gui/style/LookAndFeel.h"
#include "gui/common/Localisation.h"

namespace hum {

class DeckView : public juce::Component,
                 public juce::FileDragAndDropTarget,
                 private juce::Timer {
public:
    static constexpr int    kOverviewH = 34;
    static constexpr int    kZoomBarH = 16;
    static constexpr int    kZoomButtonW = 20;
    static constexpr int    kZoomTextW = 40;
    static constexpr double kViewSeconds = decks::DeckViewModel::kViewSeconds;

    DeckView(BrickHost& host, std::string organism, const Bindings& bound)
        : deck_(host, host.decks(), std::move(organism),
                {bound(bind::kBpm), bound(bind::kCue), bound(bind::kFile), bound(bind::kGridOffset),
                 bound(bind::kHotCuePrefix), bound(bind::kKey), bound(bind::kLoop),
                 bound(bind::kLoopIn), bound(bind::kLoopOut), bound(bind::kQuantize),
                 bound(bind::kRecord)}),
          grid_(tr("deck.set-grid", "Set Grid")), x2_("x2"), half_("/2"),
          closer_("+"), wider_("-") {
        for (auto* b : { &grid_, &x2_, &half_, &closer_, &wider_ }) {
            b->setColour(juce::TextButton::buttonColourId, Palette::panelLight);
            b->setColour(juce::TextButton::textColourOffId, Palette::text);
            addAndMakeVisible(*b);
        }
        closer_.setVisible(false);
        wider_.setVisible(false);
        grid_.setTooltip(tr("deck.anchor-the-beatgrid-beat-1", "Anchor the beatgrid (beat 1) at the playhead"));
        x2_.setTooltip(tr("deck.double-the-detected-bpm-grid", "Double the detected BPM (grid x2)"));
        half_.setTooltip(juce::String::fromUTF8("Halve the detected BPM (grid \xc3\xb7""2)"));
        closer_.setTooltip(tr("deck.zoom-the-wave-in", "Zoom the wave in"));
        wider_.setTooltip(tr("deck.zoom-the-wave-out", "Zoom the wave out"));
        closer_.onClick = [this] { deck_.zoomBy(-1); repaint(); };
        wider_.onClick  = [this] { deck_.zoomBy(1); repaint(); };
        grid_.onClick = [this] { deck_.gridAtPlayhead(); repaint(); };
        x2_.onClick   = [this] { deck_.scaleBpm(2.0); repaint(); };
        half_.onClick = [this] { deck_.scaleBpm(0.5); repaint(); };
        startTimerHz(20);
    }

    void resized() override {
        auto r = getLocalBounds();
        overview_ = r.removeFromTop(kOverviewH);
        main_ = r;
        grid_.setBounds(getWidth() - 150, 2, 56, 18);
        x2_.setBounds(getWidth() - 60, 2, 28, 18);
        half_.setBounds(getWidth() - 30, 2, 28, 18);
        wider_.setBounds(main_.getX() + 4, main_.getBottom() - kZoomBarH - 4, kZoomButtonW, kZoomBarH);
        closer_.setBounds(wider_.getRight() + kZoomTextW, wider_.getY(), kZoomButtonW, kZoomBarH);
    }

    void paint(juce::Graphics& g) override {
        g.setColour(Palette::panel.darker(0.4f));
        g.fillRect(getLocalBounds());

        const auto w = deck_.window();
        const int64_t len = w.length;
        const auto peaks = deck_.peaks();
        const int64_t pos = w.position;

        if (len <= 0) {
            g.setColour(Palette::textDim);
            g.setFont(juce::FontOptions(12.0f));
            g.drawText(tr("deck.drop-media-files-here", "Drop media files here"), getLocalBounds(), juce::Justification::centred);
            return;
        }

        const double span = w.span;
        const double left = w.left;
        const auto& p = deck_.params();

        if (!peaks.empty())
            paintWave(g, overview_, peaks, 0.0, (double) len, len,
                      Palette::accent.withAlpha(alpha::mid));
        auto ovX = [&](double s) { return overview_.getX() + (float) (s / (double) len) * overview_.getWidth(); };
        g.setColour(Palette::text.withAlpha(alpha::veil));
        g.fillRect(juce::Rectangle<float>(ovX(left), (float) overview_.getY(),
                                          ovX(left + span) - ovX(left), (float) overview_.getHeight()));
        g.setColour(juce::Colours::white);
        g.drawVerticalLine((int) ovX((double) pos), (float) overview_.getY(), (float) overview_.getBottom());

        g.setColour(Palette::panel.darker(0.6f));
        g.fillRect(main_);
        paintDetail(g, main_, w, peaks, Palette::accent);
        auto mnX = [&](double s) { return main_.getX() + (float) ((s - left) / span) * main_.getWidth(); };

        if (deck_.value(p.loop) >= 0.5) {
            const double li = deck_.value(p.loopIn), lo = deck_.value(p.loopOut);
            if (lo > li) { g.setColour(Palette::accent.withAlpha(alpha::mist));
                g.fillRect(juce::Rectangle<float>(mnX(li), (float) main_.getY(), mnX(lo) - mnX(li), (float) main_.getHeight())); }
        }
        for (const auto& [s, down] : deck_.beatLines(w)) {
            g.setColour(down ? Palette::text.withAlpha(alpha::mid) : Palette::border.withAlpha(alpha::dim));
            g.drawVerticalLine((int) mnX(s), (float) main_.getY(), (float) main_.getBottom());
        }
        auto inView = [&w](double s) { return w.shows(s); };
        const double cue = deck_.value(p.cue);
        if (cue > 0 && inView(cue)) {
            g.setColour(juce::Colours::orange);
            g.drawVerticalLine((int) mnX(cue), (float) main_.getY(), (float) main_.getBottom());
        }
        g.setFont(juce::FontOptions(9.0f));
        for (int i = 1; i <= 8; ++i) {
            const double h = deck_.value(p.hotCuePrefix + std::to_string(i));
            if (h <= 0.0 || !inView(h)) continue;
            const float x = mnX(h);
            g.setColour(Palette::accent);
            g.drawVerticalLine((int) x, (float) main_.getY(), (float) main_.getBottom());
            g.drawText(juce::String(i), (int) x + 1, main_.getY() + 1, 9, 10, juce::Justification::topLeft);
        }

        g.setColour(juce::Colours::white);
        g.drawVerticalLine(main_.getCentreX(), (float) main_.getY(), (float) main_.getBottom());

        const auto bpm = deck_.bpmText();
        g.setColour(Palette::accent);
        g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        if (!bpm.empty()) g.drawText(juce::String(bpm) + tr("deck.bpm", " BPM"), main_.reduced(6, 4), juce::Justification::topLeft);
        const juce::String info(deck_.infoText());
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(11.0f));
        g.drawText(info, main_.reduced(6, 4), juce::Justification::topRight);
        g.setFont(juce::FontOptions(10.0f));
        g.drawText(juce::String(deck_.zoomText()),
                   juce::Rectangle<int>(wider_.getRight(), wider_.getY(), kZoomTextW, kZoomBarH),
                   juce::Justification::centred, false);
        if (deck_.recording()) {
            const auto dot = StatusDot::rec();
            dot.paint(g, dot.boundsIn(main_).withBottomY(main_.getBottom() - 6));
        }
    }

    static bool playable(const juce::String& path) {
        return decks::DeckViewModel::isAudioPath(path.toStdString())
            || isVideoFile(juce::File(path));
    }
    bool isInterestedInFileDrag(const juce::StringArray& files) override {
        for (auto& f : files) if (playable(f)) return true;
        return false;
    }
    void filesDropped(const juce::StringArray& files, int, int) override {
        for (auto& f : files) if (playable(f)) { deck_.load(f.toStdString()); break; }
        repaint();
    }

    void mouseDown(const juce::MouseEvent& e) override {
        if (overview_.contains(e.getPosition())) { drag_ = Drag::Overview; scrubOverview(e.x); return; }
        if (e.mods.isAltDown()) {
            deck_.gridAt(mainXToSample(e.x)); drag_ = Drag::None; repaint(); return;
        }
        if (e.mods.isShiftDown()) { drag_ = Drag::Loop; deck_.beginLoop(mainXToSample(e.x)); return; }
        drag_ = Drag::Scrub;
        deck_.beginScrub(e.x);
    }
    void mouseDrag(const juce::MouseEvent& e) override {
        if (drag_ == Drag::Overview) { scrubOverview(e.x); return; }
        if (drag_ == Drag::Loop) {
            deck_.loopTo(mainXToSample(e.x));
            repaint(); return;
        }
        if (drag_ == Drag::Scrub) deck_.scrubTo(e.x, main_.getWidth());
    }
    void mouseUp(const juce::MouseEvent&) override {
        if (drag_ == Drag::Scrub) deck_.endScrub();
        drag_ = Drag::None;
    }

private:
    void timerCallback() override {
        const bool zoomable = deck_.window().length > 0;
        closer_.setVisible(zoomable);
        wider_.setVisible(zoomable);
        repaint();
    }

    void paintWave(juce::Graphics& g, juce::Rectangle<int> r, const std::vector<float>& peaks,
                   double startSample, double spanSamples, int64_t len, juce::Colour col) {
        if (len <= 0 || r.getWidth() <= 0) return;
        if (peaks.empty()) {
            g.setColour(col.withMultipliedAlpha(alpha::mid));
            g.drawHorizontalLine(r.getCentreY(), (float) r.getX(), (float) r.getRight());
            return;
        }
        g.setColour(col);
        const float midY = (float) r.getCentreY();
        for (int x = 0; x < r.getWidth(); ++x) {
            const double s = startSample + (double) x / r.getWidth() * spanSamples;
            const float pk = decks::peakOfWave(peaks, s, len);
            const float hh = pk * (r.getHeight() * 0.46f);
            if (hh > 0.0f) g.drawVerticalLine(r.getX() + x, midY - hh, midY + hh);
        }
    }
    void paintDetail(juce::Graphics& g, juce::Rectangle<int> r,
                     const decks::DeckViewModel::Window& w, const std::vector<float>& summary,
                     juce::Colour col) {
        const double lo = std::max(0.0, w.left);
        const double hi = std::min((double) w.length, w.left + w.span);
        if (hi <= lo || r.getWidth() <= 0) return;
        auto atX = [&](double s) {
            return r.getX() + (int) ((s - w.left) / w.span * r.getWidth());
        };
        const int from = atX(lo);
        const int columns = std::max(1, atX(hi) - from);
        const auto detail = deck_.peaksIn(lo, hi, columns);
        if (detail.empty()) {
            paintWave(g, r, summary, w.left, w.span, w.length, col);
            return;
        }
        g.setColour(col);
        const float midY = (float) r.getCentreY();
        for (int x = 0; x < (int) detail.size(); ++x) {
            const float hh = detail[(size_t) x] * (r.getHeight() * 0.46f);
            g.drawVerticalLine(from + x, midY - hh, midY + hh);
        }
    }

    int64_t mainXToSample(int x) const { return deck_.sampleAt(x, main_.getX(), main_.getWidth()); }
    void scrubOverview(int x) { deck_.seekOverview(x, overview_.getX(), overview_.getWidth()); }

    decks::DeckViewModel deck_;
    juce::TextButton grid_, x2_, half_, closer_, wider_;
    juce::Rectangle<int> overview_, main_;
    enum class Drag { None, Overview, Loop, Scrub } drag_ = Drag::None;
};

}
