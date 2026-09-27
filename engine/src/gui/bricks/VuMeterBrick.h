// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/Colours.h"
#include "gui/bricks/PolledBrick.h"
#include "gui/host/BrickHost.h"
#include "hum/Organism.h"
#include "gui/style/LookAndFeel.h"
#include "gui/editor/readouts/MeterReadings.h"
#include "gui/common/Localisation.h"

namespace hum {

class VuMeterView : public PolledBrick {
public:
    VuMeterView(BrickHost& host, std::string name)
        : PolledBrick(host, std::move(name)) {
    }

    void reloadValues() override {}
    void refreshAutomatedValues() override {}
    int preferredContentWidth() const override { return 269; }
    int preferredContentHeight(int) const override { return kPaneH + 2 * kMargin; }

    void paint(juce::Graphics& g) override {
        const int paneW = (getWidth() - kGap - 2 * kMargin) / 2;
        drawPane(g, {kMargin, kMargin, paneW, kPaneH}, 0, "L");
        drawPane(g, {kMargin + paneW + kGap, kMargin, paneW, kPaneH}, 1, "R");
    }

private:
    static constexpr int kPaneH = 86, kGap = 13, kMargin = 7;

    static float sweepOfDb(float dB) {
        return juce::jlimit(0.0f, 1.0f, (dB + 48.0f) / 48.0f);
    }

    void drawPane(juce::Graphics& g, juce::Rectangle<int> r, int ch, const char* letter) {
        const auto rf = r.toFloat();
        {
            juce::ColourGradient halo(ink::vu::halo.withAlpha(alpha::mist),
                                      rf.getCentreX(), rf.getCentreY(),
                                      ink::vu::halo.withAlpha(alpha::none),
                                      rf.getCentreX(), rf.getCentreY() - rf.getWidth() * 0.75f, true);
            g.setGradientFill(halo);
            g.fillRect(rf.expanded((float) kMargin));
        }
        juce::Path pane;
        pane.addRoundedRectangle(rf, 6.0f);
        g.saveState();
        g.reduceClipRegion(pane);
        {
            juce::ColourGradient paper(ink::vu::paperTop, 0.0f, rf.getY(),
                                       ink::vu::paperBottom, 0.0f, rf.getBottom(), false);
            g.setGradientFill(paper);
            g.fillRect(rf);
            juce::ColourGradient lamp(ink::vu::lampWash.withAlpha(alpha::scrim),
                                      rf.getCentreX(), rf.getBottom() + rf.getHeight() * 0.35f,
                                      ink::vu::lampWash.withAlpha(alpha::none),
                                      rf.getCentreX(), rf.getY(), true);
            g.setGradientFill(lamp);
            g.fillRect(rf);
        }
        const float px = rf.getCentreX();
        const float py = rf.getY() + rf.getHeight() * 1.545f;
        const float R = rf.getHeight() * 1.136f;
        const auto ink = hum::ink::vu::print, red = hum::ink::vu::printHot;
        auto theta = [&](float dB) {
            return juce::degreesToRadians(-47.0f + 94.0f * sweepOfDb(dB));
        };
        auto tip = [&](float th, float rad) {
            return juce::Point<float>(px + std::sin(th) * rad, py - std::cos(th) * rad);
        };
        {
            juce::Path arc;
            arc.addCentredArc(px, py, R * 0.92f, R * 0.92f, 0.0f, theta(-6.0f), theta(0.0f), true);
            g.setColour(red.withAlpha(alpha::heavy));
            g.strokePath(arc, juce::PathStrokeType(2.5f));
        }
        static const float marks[] = {-42, -36, -30, -24, -18, -12, -9, -6, -3, 0};
        static const bool major[] = {true, false, true, false, true, true, false, true, false, true};
        g.setFont(juce::FontOptions(6.5f));
        for (int i = 0; i < 10; ++i) {
            const float th = theta(marks[i]);
            const auto col = marks[i] >= -6.0f ? red : ink;
            g.setColour(col);
            const float r1 = R * (major[i] ? 0.86f : 0.885f);
            g.drawLine({tip(th, r1), tip(th, R * 0.93f)}, 1.0f);
            if (major[i]) {
                const auto tp = tip(th, R * 0.78f);
                g.drawText(juce::String((int) marks[i]),
                           (int) tp.x - 8, (int) tp.y - 4, 16, 8, juce::Justification::centred);
            }
        }
        g.setColour(ink);
        g.setFont(juce::FontOptions(8.0f).withStyle("Bold"));
        g.drawText(tr("vu-meter.db", "dB"), r.withTrimmedTop((int) (rf.getHeight() * 0.66f)),
                   juce::Justification::centredTop, false);
        g.setFont(juce::FontOptions(7.0f));
        g.drawText(letter, r.getX() + 7, r.getY() + 6, 12, 9, juce::Justification::left);
        {
            const float th = juce::degreesToRadians(-47.0f + 94.0f * reading_.needle(ch));
            g.setColour(ink::vu::needleShadow.withAlpha(alpha::muted));
            g.drawLine({tip(th, R * 0.18f).translated(1, 1), tip(th, R * 0.92f).translated(1, 1)}, 1.4f);
            g.setColour(ink::vu::needle);
            g.drawLine({tip(th, R * 0.18f), tip(th, R * 0.92f)}, 1.6f);
        }
        {
            const float lx = rf.getRight() - 16.0f, ly = rf.getY() + 12.0f;
            if (reading_.lamp(ch) > 0.01f) {
                juce::ColourGradient bloom(ink::vu::peakLampLit.withAlpha(0.55f * reading_.lamp(ch)),
                                           lx, ly, ink::vu::peakLampLit.withAlpha(alpha::none),
                                           lx + 10.0f, ly, true);
                g.setGradientFill(bloom);
                g.fillRect(lx - 10.0f, ly - 10.0f, 20.0f, 20.0f);
                g.setColour(ink::vu::peakLampLit.withAlpha(0.4f + 0.6f * reading_.lamp(ch)));
            } else {
                g.setColour(ink::vu::peakLampDark);
            }
            g.fillEllipse(lx - 3.0f, ly - 3.0f, 6.0f, 6.0f);
        }
        {
            juce::Path sheen;
            sheen.startNewSubPath(rf.getX(), rf.getY());
            sheen.lineTo(rf.getX() + rf.getWidth() * 0.42f, rf.getY());
            sheen.lineTo(rf.getX() + rf.getWidth() * 0.18f, rf.getY() + rf.getHeight() * 0.36f);
            sheen.lineTo(rf.getX(), rf.getY() + rf.getHeight() * 0.36f);
            sheen.closeSubPath();
            g.setColour(juce::Colours::white.withAlpha(alpha::wash));
            g.fillPath(sheen);
        }
        g.restoreState();
        g.setColour(ink::vu::bezel);
        g.strokePath(pane, juce::PathStrokeType(1.5f));
    }

    void poll() override {
        if (reading_.poll(host_, name_, host_.audioAlive() && !host_.bypassed(name_),
                          juce::Time::getMillisecondCounter()))
            repaint();
    }

    readout::VuNeedles reading_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VuMeterView)
};

}
