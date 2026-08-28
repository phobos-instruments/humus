#pragma once
#include <set>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/EngineHost.h"
#include "gui/LookAndFeel.h"

namespace hum {

class DeviceStripView : public juce::Component {
public:
    static constexpr int kHeight = 44;

    DeviceStripView(EngineHost& host, std::string name)
        : host_(host), name_(std::move(name)) {
        setInterceptsMouseClicks(false, false);
    }

    void setNote(juce::String note, juce::Colour colour) {
        note_ = std::move(note);
        noteColour_ = colour;
        repaint();
    }

    void paint(juce::Graphics& g) override {
        paintFace(g, host_, name_, getLocalBounds(), note_, noteColour_);
    }

    static void paintFace(juce::Graphics& g, EngineHost& host, const std::string& name,
                          juce::Rectangle<int> area, const juce::String& note,
                          juce::Colour noteColour = juce::Colour()) {
        const auto r = area.toFloat();
        juce::Path body;
        body.addRoundedRectangle(r.getX(), r.getY(), r.getWidth(), r.getHeight(),
                                 6.0f, 6.0f, false, false, true, true);
        g.setColour(Palette::background);
        g.fillPath(body);
        g.setColour(Palette::accent.withAlpha(0.35f));
        g.fillRect(r.withHeight(3.0f));

        std::set<int> insLit, outsLit;
        bool midiInLit = false, midiOutLit = false;
        for (const auto& c : host.model().connections) {
            if (c.dst == name) insLit.insert(c.dstInlet);
            if (c.src == name) outsLit.insert(c.srcOutlet);
        }
        for (const auto& c : host.model().midiConnections) {
            midiInLit  = midiInLit  || c.dst == name;
            midiOutLit = midiOutLit || c.src == name;
        }

        const float cy = r.getY() + 3.0f + (r.getHeight() - 3.0f) * 0.42f;
        const float labelY = cy + 7.0f;
        auto jack = [&](float x, float rad, juce::Colour c, bool lit) {
            if (lit) {
                g.setColour(c);
                g.fillEllipse(x - rad, cy - rad, rad * 2.0f, rad * 2.0f);
                g.setColour(juce::Colours::white.withAlpha(0.30f));
                g.fillEllipse(x - rad * 0.45f, cy - rad * 0.65f, rad * 0.55f, rad * 0.55f);
            } else {
                g.setColour(c.withAlpha(0.55f));
                g.drawEllipse(x - rad, cy - rad, rad * 2.0f, rad * 2.0f, 1.2f);
            }
        };
        constexpr float kDot = 3.4f, kStep = 10.0f;
        constexpr int kMaxDots = 8;
        auto cluster = [&](float x, int dir, int n, const std::set<int>& lit,
                           bool midi, bool midiLit) {
            for (int i = 0; i < juce::jmin(n, kMaxDots); ++i, x += dir * kStep)
                jack(x, kDot, Palette::textDim, lit.count(i) > 0);
            if (n > kMaxDots) {
                g.setColour(Palette::textDim);
                g.setFont(juce::FontOptions(9.0f));
                g.drawText("+" + juce::String(n - kMaxDots),
                           juce::Rectangle<float>(x - 9.0f, cy - 6.0f, 18.0f, 12.0f),
                           juce::Justification::centred, false);
                x += dir * kStep;
            }
            if (midi) { jack(x, kDot - 0.6f, Palette::midiCord(), midiLit); x += dir * kStep; }
            return x;
        };
        const int ins  = host.inletsOf(name),      outs  = host.outletsOf(name);
        const int mIns = host.midiInletsOf(name),  mOuts = host.midiOutletsOf(name);
        const float leftEnd  = cluster(r.getX() + 13.0f, +1, ins, insLit, mIns > 0, midiInLit);
        const float rightEnd = cluster(r.getRight() - 13.0f, -1, outs, outsLit, mOuts > 0, midiOutLit);

        g.setFont(juce::FontOptions(9.0f));
        g.setColour(Palette::textDim.withAlpha(0.7f));
        if (ins > 0 || mIns > 0)
            g.drawText("in", (int) r.getX() + 9, (int) labelY, 30, 11,
                       juce::Justification::centredLeft, false);
        if (outs > 0 || mOuts > 0)
            g.drawText("out", (int) r.getRight() - 39, (int) labelY, 30, 11,
                       juce::Justification::centredRight, false);

        g.setColour(noteColour.isTransparent() ? Palette::textDim : noteColour);
        g.setFont(juce::FontOptions(11.0f));
        const int tx = (int) leftEnd + 4;
        g.drawText(note, tx, area.getY() + 3, juce::jmax(20, (int) rightEnd - 4 - tx),
                   area.getHeight() - 3, juce::Justification::centred, true);
    }

private:
    EngineHost& host_;
    std::string name_;
    juce::String note_;
    juce::Colour noteColour_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeviceStripView)
};

}
