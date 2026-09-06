#pragma once
#include <cmath>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/PolledBrick.h"
#include "gui/EngineHost.h"
#include "gui/LookAndFeel.h"
#include "hum/Capabilities.h"
#include "gui/Localisation.h"

namespace hum {

class VuMeterView : public PolledBrick {
public:
    VuMeterView(EngineHost& host, std::string name)
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
            juce::ColourGradient halo(juce::Colour(0xffff9640).withAlpha(0.10f),
                                      rf.getCentreX(), rf.getCentreY(),
                                      juce::Colour(0xffff9640).withAlpha(0.0f),
                                      rf.getCentreX(), rf.getCentreY() - rf.getWidth() * 0.75f, true);
            g.setGradientFill(halo);
            g.fillRect(rf.expanded((float) kMargin));
        }
        juce::Path pane;
        pane.addRoundedRectangle(rf, 6.0f);
        g.saveState();
        g.reduceClipRegion(pane);
        {
            juce::ColourGradient paper(juce::Colour(0xfff0e4c4), 0.0f, rf.getY(),
                                       juce::Colour(0xffdccb9f), 0.0f, rf.getBottom(), false);
            g.setGradientFill(paper);
            g.fillRect(rf);
            juce::ColourGradient lamp(juce::Colour(0xffffa63a).withAlpha(0.30f),
                                      rf.getCentreX(), rf.getBottom() + rf.getHeight() * 0.35f,
                                      juce::Colour(0xffffa63a).withAlpha(0.0f),
                                      rf.getCentreX(), rf.getY(), true);
            g.setGradientFill(lamp);
            g.fillRect(rf);
        }
        const float px = rf.getCentreX();
        const float py = rf.getY() + rf.getHeight() * 1.545f;
        const float R = rf.getHeight() * 1.136f;
        const juce::Colour ink(0xff2a2216), red(0xffb23a28);
        auto theta = [&](float dB) {
            return juce::degreesToRadians(-47.0f + 94.0f * sweepOfDb(dB));
        };
        auto tip = [&](float th, float rad) {
            return juce::Point<float>(px + std::sin(th) * rad, py - std::cos(th) * rad);
        };
        {
            juce::Path arc;
            arc.addCentredArc(px, py, R * 0.92f, R * 0.92f, 0.0f, theta(-6.0f), theta(0.0f), true);
            g.setColour(red.withAlpha(0.85f));
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
            const float th = juce::degreesToRadians(-47.0f + 94.0f * pos_[ch]);
            g.setColour(juce::Colour(0xff3c2814).withAlpha(0.35f));
            g.drawLine({tip(th, R * 0.18f).translated(1, 1), tip(th, R * 0.92f).translated(1, 1)}, 1.4f);
            g.setColour(juce::Colour(0xff1c160e));
            g.drawLine({tip(th, R * 0.18f), tip(th, R * 0.92f)}, 1.6f);
        }
        {
            const float lx = rf.getRight() - 16.0f, ly = rf.getY() + 12.0f;
            if (lampLit_[ch] > 0.01f) {
                juce::ColourGradient bloom(juce::Colour(0xffff5038).withAlpha(0.55f * lampLit_[ch]),
                                           lx, ly, juce::Colour(0xffff5038).withAlpha(0.0f),
                                           lx + 10.0f, ly, true);
                g.setGradientFill(bloom);
                g.fillRect(lx - 10.0f, ly - 10.0f, 20.0f, 20.0f);
                g.setColour(juce::Colour(0xffff5038).withAlpha(0.4f + 0.6f * lampLit_[ch]));
            } else {
                g.setColour(juce::Colour(0xff3f1510));
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
            g.setColour(juce::Colours::white.withAlpha(0.06f));
            g.fillPath(sheen);
        }
        g.restoreState();
        g.setColour(juce::Colour(0xff15110d));
        g.strokePath(pane, juce::PathStrokeType(1.5f));
    }

    static constexpr float kHoldS = 1.5f;
    static constexpr float kLampTrip = 0.8913f;

    void poll() override {
        auto* src = dynamic_cast<VuSource*>(host_.liveOrganism(name_));
        const bool live = host_.audioAlive() && !host_.bypassed(name_);
        const float dt = 1.0f / 30.0f;
        bool moving = false;
        for (int c = 0; c < 2; ++c) {
            const float pk = src != nullptr && live ? src->vuPeak(c) : 0.0f;
            const float db = 20.0f * std::log10(juce::jmax(pk, 1.0e-5f));
            const float target = pk <= 0.0f ? 0.0f
                               : juce::jlimit(0.0f, 1.04f, (db + 48.0f) / 48.0f);
            const float w = juce::MathConstants<float>::twoPi * 2.1f;
            const float z = 0.62f;
            vel_[c] += (w * w * (target - pos_[c]) - 2.0f * z * w * vel_[c]) * dt;
            pos_[c] += vel_[c] * dt;
            if (pk >= kLampTrip) lampUntil_[c] = juce::Time::getMillisecondCounter() + (juce::uint32) (kHoldS * 1000.0f);
            const bool lit = juce::Time::getMillisecondCounter() < lampUntil_[c];
            const float lampTarget = lit ? 1.0f : 0.0f;
            lampLit_[c] += (lampTarget - lampLit_[c]) * (lit ? 1.0f : 0.28f);
            if (target <= 0.0f && std::abs(pos_[c]) < 0.004f && std::abs(vel_[c]) < 0.01f
                && lampLit_[c] < 0.01f) {
                pos_[c] = 0.0f; vel_[c] = 0.0f; lampLit_[c] = 0.0f;
            }
            if (std::abs(pos_[c] - shown_[c]) > 0.0025f || lampLit_[c] > 0.01f) moving = true;
        }
        if (moving) {
            shown_[0] = pos_[0]; shown_[1] = pos_[1];
            repaint();
        }
    }

    float pos_[2] = {0, 0}, vel_[2] = {0, 0}, shown_[2] = {0, 0};
    float lampLit_[2] = {0, 0};
    juce::uint32 lampUntil_[2] = {0, 0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VuMeterView)
};

}
