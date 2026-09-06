#pragma once
#include <cmath>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/PolledBrick.h"
#include "gui/EngineHost.h"
#include "gui/LookAndFeel.h"
#include "hum/Capabilities.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class FieldScopeView : public PolledBrick {
public:
    FieldScopeView(EngineHost& host, std::string name)
        : PolledBrick(host, std::move(name)) {
    }

    void reloadValues() override {}
    void refreshAutomatedValues() override {}
    int preferredContentWidth() const override { return 165; }
    int preferredContentHeight(int) const override { return 140; }

    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        g.setColour(juce::Colour(0xff12100c));
        g.fillRoundedRectangle(r, 4.0f);
        g.setColour(juce::Colour(0xff0c0a08));
        g.drawRoundedRectangle(r.reduced(0.5f), 4.0f, 1.0f);

        const float cx = r.getCentreX();
        const float floorY = r.getBottom() - 10.0f;
        g.setColour(Palette::text.withAlpha(0.12f));
        g.drawLine(cx, r.getY() + 4.0f, cx, floorY, 1.0f);
        g.drawLine(cx, floorY, r.getX() + 6.0f, r.getY() + 8.0f, 1.0f);
        g.drawLine(cx, floorY, r.getRight() - 6.0f, r.getY() + 8.0f, 1.0f);
        g.setColour(Palette::textDim.withAlpha(0.6f));
        g.setFont(juce::FontOptions(8.0f));
        g.drawText("L", (int) r.getX() + 5, (int) r.getY() + 4, 10, 10, juce::Justification::left);
        g.drawText("R", (int) r.getRight() - 15, (int) r.getY() + 4, 10, 10, juce::Justification::right);

        if (haveCloud_) {
            const float span = r.getWidth() * 0.46f;
            for (int i = 0; i < pairs_; ++i) {
                const float L = lr_[2 * i], R = lr_[2 * i + 1];
                const float side = (L - R) * kSqrtHalfF;
                const float mid = (L + R) * kSqrtHalfF;
                const float px = cx + juce::jlimit(-1.2f, 1.2f, side) * span;
                const float py = floorY - juce::jlimit(-0.1f, 1.3f, std::abs(mid)) * (floorY - r.getY() - 8.0f) * 0.8f;
                const float t = (float) i / (float) juce::jmax(1, pairs_ - 1);
                const float a = (0.10f + 0.65f * t * t) * cloudFade_;
                if (a < 0.02f) continue;
                g.setColour(Palette::accent.withAlpha(a));
                const float rad = t > 0.9f ? 1.6f : 1.1f;
                g.fillEllipse(px - rad, py - rad, rad * 2.0f, rad * 2.0f);
            }
        }

        const juce::Rectangle<float> strip(r.getX() + 2.0f, r.getBottom() - 8.0f,
                                           r.getWidth() - 4.0f, 6.0f);
        g.setColour(juce::Colour(0xff0e0c09));
        g.fillRect(strip);
        g.setColour(Palette::text.withAlpha(0.8f));
        g.fillRect(cx - 0.5f, strip.getY(), 1.0f, strip.getHeight());
        if (haveCloud_ && std::abs(corr_) > 0.01f) {
            const float w = std::abs(corr_) * (strip.getWidth() * 0.5f - 2.0f) * cloudFade_;
            if (corr_ >= 0.0f) {
                g.setColour(Palette::accent.withAlpha(0.8f * cloudFade_));
                g.fillRect(cx + 0.5f, strip.getY() + 1.0f, w, strip.getHeight() - 2.0f);
            } else {
                g.setColour(Palette::textDim.withAlpha(0.8f * cloudFade_));
                g.fillRect(cx - 0.5f - w, strip.getY() + 1.0f, w, strip.getHeight() - 2.0f);
            }
        }
    }

private:
    void poll() override {
        auto* src = dynamic_cast<StereoFieldSource*>(host_.liveOrganism(name_));
        if (src == nullptr) return;
        const unsigned stamp = src->fieldStamp();
        if (stamp != lastStamp_) {
            lastStamp_ = stamp;
            pairs_ = src->fieldRead(lr_, kMaxPairs);
            corr_ = src->fieldCorrelation();
            cloudFade_ = 1.0f;
            haveCloud_ = pairs_ > 0;
            repaint();
        } else if (haveCloud_) {
            cloudFade_ *= 0.82f;
            if (cloudFade_ < 0.03f) { haveCloud_ = false; cloudFade_ = 0.0f; }
            repaint();
        }
    }

    static constexpr int kMaxPairs = StereoFieldSource::kFieldPairs;
    float lr_[2 * kMaxPairs] = {};
    int pairs_ = 0;
    float corr_ = 0.0f;
    float cloudFade_ = 0.0f;
    bool haveCloud_ = false;
    unsigned lastStamp_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FieldScopeView)
};

}
