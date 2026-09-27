// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/AutomateMenu.h"
#include "gui/editor/BrickBindings.h"
#include "gui/editor/ParamRanges.h"
#include "gui/style/Colours.h"
#include "gui/style/Contrast.h"
#include "gui/bricks/PolledBrick.h"
#include "gui/host/BrickHost.h"
#include "hum/Organism.h"
#include "gui/style/LookAndFeel.h"
#include "gui/editor/files/SoundMapModel.h"
#include "gui/common/Localisation.h"

namespace hum {

class SoundMapView : public PolledBrick {
public:
    SoundMapView(BrickHost& host, std::string organism, std::string paramX, std::string paramY,
                 const Bindings& bound)
        : PolledBrick(host, organism, 1),
          map_(host, organism, std::move(paramX), std::move(paramY), bound(bind::kSpray),
               bound(bind::kFilePrefix), bound(bind::kFit)),
          sizeParam_(bound(bind::kGrainSize)), spreadParam_(bound(bind::kStereoSpread)),
          pitchParam_(bound(bind::kPitch)) {
        setMouseCursor(juce::MouseCursor::CrosshairCursor);
    }

    void reloadValues() override { map_.invalidate(); repaint(); }
    void refreshAutomatedValues() override { repaint(); }
    int preferredContentWidth() const override { return 368; }
    int preferredContentHeight(int) const override { return 368; }

    static juce::Colour ground() {
        return contrast::isLight(Palette::panel) ? Palette::panel.darker(0.05f)
                                                 : Palette::background.darker(0.25f);
    }

    void paint(juce::Graphics& g) override {
        const float scale = juce::jlimit(1.0f, 3.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
        pullPoints(scale);
        const float w = (float) getWidth(), h = (float) getHeight();
        g.fillAll(ground());

        g.setColour(Palette::border.withAlpha(alpha::mist));
        for (int e = 1; e < 8; ++e) {
            if (e % 2 == 0) continue;
            g.drawVerticalLine((int) (w * e / 8.0f), 0.0f, h);
            g.drawHorizontalLine((int) (h * e / 8.0f), 0.0f, w);
        }
        g.setColour(Palette::border.withAlpha(alpha::mid));
        for (const int q : {1, 3}) {
            g.drawVerticalLine((int) (w * q / 4.0f), 0.0f, h);
            g.drawHorizontalLine((int) (h * q / 4.0f), 0.0f, w);
        }
        g.setColour(Palette::border);
        g.drawVerticalLine((int) (w * 0.5f), 0.0f, h);
        g.drawHorizontalLine((int) (h * 0.5f), 0.0f, w);
        g.setFont(juce::FontOptions(9.0f));
        g.setColour(Palette::textDim.withAlpha(alpha::heavy));
        for (int q = 1; q < 4; ++q) {
            const auto& win = map_.window();
            const auto label = juce::String((int) std::lround(win.paramX(q / 4.0) * 100.0));
            const auto down = juce::String((int) std::lround(win.paramY(1.0 - q / 4.0) * 100.0));
            g.drawText(label, (int) (w * q / 4.0f) - 12, (int) h - 13, 24, 11,
                       juce::Justification::centred);
            g.drawText(down, 3, (int) (h * q / 4.0f) - 5, 20, 11,
                       juce::Justification::centredLeft);
        }

        if (dots_.isValid())
            g.drawImageTransformed(dots_, juce::AffineTransform::scale(1.0f / scale));
        g.setColour(Palette::border);
        g.drawRect(getLocalBounds());

        if (map_.points().empty()) {
            g.setColour(Palette::textDim);
            g.setFont(juce::FontOptions(13.0f));
            g.drawFittedText(juce::String::fromUTF8(
                                 "Play something in, or load files\n\xe2\x86\x92 then drag to explore"),
                             getLocalBounds().reduced(10), juce::Justification::centred, 2, 1.0f);
            return;
        }

        const auto& view = map_.window();
        const float cx = (float) view.viewX(map_.x()) * w;
        const float cy = (1.0f - (float) view.viewY(map_.y())) * h;

        paintGlows(g, w, h);

        const double zoomX = view.x1 > view.x0 ? 1.0 / (view.x1 - view.x0) : 1.0;
        const float spray = (float) (map_.spray() * zoomX) * w;
        if (spray > 1.0f) {
            g.setColour(Palette::accent.withAlpha(alpha::mist));
            g.fillEllipse(cx - spray, cy - spray, spray * 2.0f, spray * 2.0f);
            g.setColour(Palette::accent.withAlpha(alpha::muted));
            g.drawEllipse(cx - spray, cy - spray, spray * 2.0f, spray * 2.0f, ringThickness());
        }
        const float reach = std::max(spray, kCursorLeastReach);
        paintSpreadWings(g, cx, cy, reach);
        paintPitchSpan(g, cx, cy, reach);

        g.setColour(Palette::accent.withAlpha(alpha::scrim));
        g.drawVerticalLine((int) cx, 0.0f, h);
        g.drawHorizontalLine((int) cy, 0.0f, w);

        g.setColour(ground().darker(0.35f).withAlpha(alpha::heavy));
        g.drawLine(cx - 10.0f, cy, cx + 10.0f, cy, 3.6f);
        g.drawLine(cx, cy - 10.0f, cx, cy + 10.0f, 3.6f);
        g.fillEllipse(cx - 5.0f, cy - 5.0f, 10.0f, 10.0f);
        g.setColour(Palette::accent);
        g.drawLine(cx - 9.0f, cy, cx + 9.0f, cy, 1.6f);
        g.drawLine(cx, cy - 9.0f, cx, cy + 9.0f, 1.6f);
        g.fillEllipse(cx - 3.0f, cy - 3.0f, 6.0f, 6.0f);

        const juce::String readout(map_.readout());
        const juce::Rectangle<int> chip((int) w - 64, 4, 60, 16);
        g.setColour(Palette::panel.withAlpha(alpha::heavy));
        g.fillRoundedRectangle(chip.toFloat(), 3.0f);
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(10.5f));
        g.drawText(readout, chip, juce::Justification::centred);
    }

    int glowCountForTest() const {
        return (int) map_.trail().glows(juce::Time::getMillisecondCounterHiRes() * 0.001).size();
    }

    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu()) { showMenu(e.getScreenPosition()); return; }
        map_.begin();
        apply(e, true);
    }
    void mouseDrag(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu()) return;
        apply(e, false);
    }
    void mouseUp(const juce::MouseEvent& e) override {
        if (!e.mods.isPopupMenu()) map_.end();
    }

private:
    void poll() override {
        map_.collectTakes();
        map_.pullGrains(juce::Time::getMillisecondCounterHiRes() * 0.001);
        repaint();
    }

    double normalised(const std::string& param, double value) {
        const auto [lo, hi] = paramRange(host_, name_, param);
        return hi > lo ? std::clamp((value - lo) / (hi - lo), 0.0, 1.0) : 0.0;
    }

    float ringThickness() {
        if (sizeParam_.empty()) return 1.0f;
        const double mid = 0.5 * (host_.liveParamValue(name_, sizeParam_) + host_.liveParamMax(name_, sizeParam_));
        return 1.0f + kRingThickest * (float) std::sqrt(normalised(sizeParam_, mid));
    }

    void paintGlows(juce::Graphics& g, float w, float h) {
        const auto& view = map_.window();
        for (const auto& glow : map_.trail().glows(juce::Time::getMillisecondCounterHiRes() * 0.001)) {
            const float gx = (float) view.viewX(glow.x) * w;
            const float gy = (1.0f - (float) view.viewY(glow.y)) * h;
            const float r = kGlowLeast + kGlowGrowth * glow.life;
            const auto ink = fileColour(glow.file).brighter(0.4f);
            g.setColour(ink.withAlpha(alpha::mist * glow.life));
            g.fillEllipse(gx - r * 2.0f, gy - r * 2.0f, r * 4.0f, r * 4.0f);
            g.setColour(ink.withAlpha(glow.life));
            g.fillEllipse(gx - r * 0.5f, gy - r * 0.5f, r, r);
        }
    }

    void paintSpreadWings(juce::Graphics& g, float cx, float cy, float reach) {
        if (spreadParam_.empty()) return;
        const float spread = (float) normalised(spreadParam_, host_.liveParamValue(name_, spreadParam_));
        if (spread <= 0.0f) return;
        const float out = reach + kWingGap + kWingTravel * spread;
        g.setColour(Palette::accent.withAlpha(alpha::muted));
        for (const float side : {-1.0f, 1.0f}) {
            juce::Path wing;
            wing.addCentredArc(cx, cy, out, out, 0.0f, side * juce::MathConstants<float>::halfPi - kWingArc,
                               side * juce::MathConstants<float>::halfPi + kWingArc, true);
            g.strokePath(wing, juce::PathStrokeType(1.4f + spread));
        }
    }

    void paintPitchSpan(juce::Graphics& g, float cx, float cy, float reach) {
        if (pitchParam_.empty()) return;
        const auto [lo, hi] = paramRange(host_, name_, pitchParam_);
        const double half = 0.5 * (hi - lo);
        if (half <= 0.0) return;
        const double mid = 0.5 * (lo + hi);
        const float from = (float) ((host_.liveParamValue(name_, pitchParam_) - mid) / half);
        const float to = (float) ((host_.liveParamMax(name_, pitchParam_) - mid) / half);
        if (std::abs(from) < kPitchQuiet && std::abs(to) < kPitchQuiet) return;
        const float x = cx + reach - kWingGap;
        const float y0 = cy - from * kPitchReach * reach, y1 = cy - to * kPitchReach * reach;
        g.setColour(Palette::accent.withAlpha(alpha::strong));
        if (std::abs(y1 - y0) < 2.0f) {
            const float dir = from > 0.0f ? -1.0f : 1.0f;
            juce::Path tip;
            tip.addTriangle(x - kPitchTip, y0 - dir * kPitchTip, x + kPitchTip, y0 - dir * kPitchTip, x, y0 + dir * kPitchTip);
            g.fillPath(tip);
            g.drawLine(x, cy, x, y0, 1.4f);
        } else {
            g.drawLine(x, y0, x, y1, 3.0f);
            g.drawLine(x - kPitchTip, y0, x + kPitchTip, y0, 1.2f);
            g.drawLine(x - kPitchTip, y1, x + kPitchTip, y1, 1.2f);
        }
    }

    void apply(const juce::MouseEvent& e, bool first) {
        map_.place(e.position.x, e.position.y, getWidth(), getHeight(), first);
        repaint();
    }

    void showMenu(juce::Point<int> screen) {
        juce::PopupMenu m;
        m.addSectionHeader(tr("sound-map.cursor", "Cursor"));
        m.addItem(1, tr("sound-map.midi-learn-x-then-y", "MIDI Learn X, then Y..."));
        m.addSeparator();
        m.addItem(2, tr("sound-map.automate-midi-for-x", "Automate / MIDI for X..."));
        m.addItem(3, tr("sound-map.automate-midi-for-y", "Automate / MIDI for Y..."));
        m.showMenuAsync(juce::PopupMenu::Options()
                            .withTargetScreenArea({screen.x, screen.y, 1, 1}),
                        [this, screen](int r) {
            if (r == 1) learnBothAxes();
            else if (r == 2 || r == 3)
                showAutomateMenu(host_, name_, r == 2 ? map_.paramX() : map_.paramY(), screen,
                                 [this] { if (onAutomationChanged) onAutomationChanged(); });
        });
    }

    void learnBothAxes() {
        auto& learner = MidiLearner::instance();
        const auto rx = paramRange(host_, name_, map_.paramX());
        learner.arm(host_, name_, map_.paramX(), rx.first, rx.second);
        learner.onCaptured = [&host = host_, cn = name_, py = map_.paramY()] {
            const auto ry = paramRange(host, cn, py);
            quickMapMidi(host, cn, py);
        };
    }

    void pullPoints(float scale) {
        const int iw = juce::jmax(1, (int) std::lround(getWidth() * scale));
        const int ih = juce::jmax(1, (int) std::lround(getHeight() * scale));
        if (!map_.refreshPoints(dots_.getWidth() != iw || dots_.getHeight() != ih)) return;

        dots_ = juce::Image(juce::Image::ARGB, iw, ih, true);
        juce::Graphics dg(dots_);
        const auto& view = map_.window();
        for (const auto& p : map_.points()) {
            const float r = (p.file == SoundMapSource::kFreshLiveFile ? 4.2f : 1.9f) * scale;
            dg.setColour(fileColour(p.file));
            dg.fillEllipse((float) view.viewX(p.x) * (float) iw - r * 0.5f,
                           (1.0f - (float) view.viewY(p.y)) * (float) ih - r * 0.5f, r, r);
        }
    }

    static juce::Colour fileColour(int file) {
        static const float rot[4] = {0.0f, 0.14f, 0.42f, 0.68f};
        if (file == SoundMapSource::kFreshLiveFile) return Palette::text;
        if (file == SoundMapSource::kLiveFile) return Palette::text.withAlpha(alpha::mid);
        return Palette::accent.withRotatedHue(rot[file & 3]).withAlpha(alpha::strong);
    }

    static constexpr float kCursorLeastReach = 14.0f;
    static constexpr float kRingThickest = 3.0f;
    static constexpr float kGlowLeast = 2.5f;
    static constexpr float kGlowGrowth = 4.5f;
    static constexpr float kWingGap = 6.0f;
    static constexpr float kWingTravel = 18.0f;
    static constexpr float kWingArc = 0.45f;
    static constexpr float kPitchQuiet = 0.01f;
    static constexpr float kPitchTip = 3.5f;
    static constexpr float kPitchReach = 2.0f;

    files::SoundMapModel map_;
    std::string sizeParam_, spreadParam_, pitchParam_;
    juce::Image dots_;
};

}
