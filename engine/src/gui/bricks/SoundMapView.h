// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/AutomateMenu.h"
#include "gui/editor/BrickBindings.h"
#include "gui/style/Colours.h"
#include "gui/bricks/PolledBrick.h"
#include "gui/host/BrickHost.h"
#include "hum/Organism.h"
#include "gui/style/LookAndFeel.h"
#include "hum/caps/Files.h"
#include "hum/caps/Samples.h"
#include "gui/common/Localisation.h"

namespace hum {

class SoundMapView : public PolledBrick {
public:
    SoundMapView(BrickHost& host, std::string organism, std::string paramX, std::string paramY,
                 const Bindings& bound)
        : PolledBrick(host, std::move(organism), 2),
          px_(std::move(paramX)), py_(std::move(paramY)), spray_(bound(bind::kSpray)),
          filePrefix_(bound(bind::kFilePrefix)) {
        setMouseCursor(juce::MouseCursor::CrosshairCursor);
    }

    void reloadValues() override { generation_ = ~0u; repaint(); }
    void refreshAutomatedValues() override { repaint(); }
    int preferredContentWidth() const override { return 368; }
    int preferredContentHeight(int) const override { return 368; }

    void paint(juce::Graphics& g) override {
        const float scale = juce::jlimit(1.0f, 3.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
        pullPoints(scale);
        const float w = (float) getWidth(), h = (float) getHeight();
        g.fillAll(Palette::background.darker(0.25f));

        g.setColour(Palette::border.withAlpha(alpha::dim));
        for (int q = 1; q < 4; ++q) {
            g.drawVerticalLine((int) (w * q / 4.0f), 0.0f, h);
            g.drawHorizontalLine((int) (h * q / 4.0f), 0.0f, w);
        }
        g.setColour(Palette::border);
        for (int t = 1; t < 8; ++t) {
            g.drawVerticalLine((int) (w * t / 8.0f), h - 5.0f, h);
            g.drawHorizontalLine((int) (h * t / 8.0f), 0.0f, 5.0f);
        }
        g.setFont(juce::FontOptions(9.0f));
        g.setColour(Palette::textDim.withAlpha(alpha::heavy));
        for (int q = 1; q < 4; ++q) {
            const auto label = juce::String(q * 25);
            g.drawText(label, (int) (w * q / 4.0f) - 12, (int) h - 13, 24, 11,
                       juce::Justification::centred);
            g.drawText(label, 3, (int) (h * (4 - q) / 4.0f) - 5, 20, 11,
                       juce::Justification::centredLeft);
        }

        if (dots_.isValid())
            g.drawImageTransformed(dots_, juce::AffineTransform::scale(1.0f / scale));
        g.setColour(Palette::border);
        g.drawRect(getLocalBounds());

        if (points_.empty()) {
            g.setColour(Palette::textDim);
            g.setFont(juce::FontOptions(13.0f));
            g.drawText(juce::String::fromUTF8(
                           "Load corpus files \xe2\x86\x92  grains land here, drag to explore"),
                       getLocalBounds().reduced(10), juce::Justification::centred, true);
            return;
        }

        const double xv = host_.liveParamValue(name_, px_);
        const double yv = host_.liveParamValue(name_, py_);
        const float cx = (float) xv * w;
        const float cy = (1.0f - (float) yv) * h;

        const float spray = (float) host_.liveParamValue(name_, spray_) * w;
        if (spray > 1.0f) {
            g.setColour(Palette::accent.withAlpha(alpha::mist));
            g.fillEllipse(cx - spray, cy - spray, spray * 2.0f, spray * 2.0f);
            g.setColour(Palette::accent.withAlpha(alpha::muted));
            g.drawEllipse(cx - spray, cy - spray, spray * 2.0f, spray * 2.0f, 1.0f);
        }

        g.setColour(Palette::accent.withAlpha(alpha::scrim));
        g.drawVerticalLine((int) cx, 0.0f, h);
        g.drawHorizontalLine((int) cy, 0.0f, w);

        g.setColour(Palette::background.darker(0.6f).withAlpha(alpha::heavy));
        g.drawLine(cx - 10.0f, cy, cx + 10.0f, cy, 3.6f);
        g.drawLine(cx, cy - 10.0f, cx, cy + 10.0f, 3.6f);
        g.fillEllipse(cx - 5.0f, cy - 5.0f, 10.0f, 10.0f);
        g.setColour(Palette::accent);
        g.drawLine(cx - 9.0f, cy, cx + 9.0f, cy, 1.6f);
        g.drawLine(cx, cy - 9.0f, cx, cy + 9.0f, 1.6f);
        g.fillEllipse(cx - 3.0f, cy - 3.0f, 6.0f, 6.0f);

        const auto readout = juce::String((int) std::lround(xv * 100.0)) + " , "
                           + juce::String((int) std::lround(yv * 100.0));
        const juce::Rectangle<int> chip((int) w - 64, 4, 60, 16);
        g.setColour(Palette::panel.withAlpha(alpha::heavy));
        g.fillRoundedRectangle(chip.toFloat(), 3.0f);
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(10.5f));
        g.drawText(readout, chip, juce::Justification::centred);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu()) { showMenu(e.getScreenPosition()); return; }
        host_.beginParamDrag(name_, px_);
        apply(e, true);
    }
    void mouseDrag(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu()) return;
        apply(e, false);
    }
    void mouseUp(const juce::MouseEvent& e) override {
        if (!e.mods.isPopupMenu()) host_.endParamDrag();
    }

private:
    void poll() override {
        if (auto* cap = dynamic_cast<LiveCaptureSource*>(host_.liveOrganism(name_))) {
            LiveCaptureSource::Take take;
            if (cap->fetchCompletedTake(take))
                host_.setParamText(name_, filePrefix_ + std::to_string(take.slot), take.path);
        }
        repaint();
    }

    void apply(const juce::MouseEvent& e, bool first) {
        const double x = juce::jlimit(0.0, 1.0, e.position.x / (double) juce::jmax(1, getWidth()));
        const double y = juce::jlimit(0.0, 1.0, 1.0 - e.position.y / (double) juce::jmax(1, getHeight()));
        if (first) {
            host_.editParam(name_, px_, x);
            host_.editParam(name_, py_, y);
        } else {
            host_.setParam(name_, px_, x);
            host_.setParam(name_, py_, y);
        }
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
                showAutomateMenu(host_, name_, r == 2 ? px_ : py_, screen,
                                 [this] { if (onAutomationChanged) onAutomationChanged(); });
        });
    }

    void learnBothAxes() {
        auto& learner = MidiLearner::instance();
        const auto rx = paramRange(host_, name_, px_);
        learner.arm(host_, name_, px_, rx.first, rx.second);
        learner.onCaptured = [&host = host_, cn = name_, py = py_] {
            const auto ry = paramRange(host, cn, py);
            MidiLearner::instance().arm(host, cn, py, ry.first, ry.second);
        };
    }

    void pullPoints(float scale) {
        auto* src = dynamic_cast<SoundMapSource*>(host_.liveOrganism(name_));
        const unsigned gen = src ? src->mapGeneration() : 0;
        const int iw = juce::jmax(1, (int) std::lround(getWidth() * scale));
        const int ih = juce::jmax(1, (int) std::lround(getHeight() * scale));
        if (gen == generation_ && dots_.getWidth() == iw && dots_.getHeight() == ih) return;
        generation_ = gen;
        points_ = src ? src->mapPoints() : std::vector<SoundMapSource::MapPoint>{};

        dots_ = juce::Image(juce::Image::ARGB, iw, ih, true);
        juce::Graphics dg(dots_);
        const float r = 1.9f * scale;
        for (const auto& p : points_) {
            dg.setColour(fileColour(p.file));
            dg.fillEllipse(p.x * (float) iw - r * 0.5f,
                           (1.0f - p.y) * (float) ih - r * 0.5f, r, r);
        }
    }

    static juce::Colour fileColour(int file) {
        static const float rot[4] = {0.0f, 0.14f, 0.42f, 0.68f};
        return Palette::accent.withRotatedHue(rot[file & 3]).withAlpha(alpha::strong);
    }

    std::string px_, py_;
    std::string spray_, filePrefix_;
    std::vector<SoundMapSource::MapPoint> points_;
    juce::Image dots_;
    unsigned generation_ = ~0u;
};

}
