// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/BrickBindings.h"
#include "gui/style/Colours.h"
#include "gui/host/BrickHost.h"
#include "gui/host/EngineHostFiles.h"
#include "gui/style/LookAndFeel.h"
#include "gui/bricks/PolledBrick.h"
#include "hum/dsp/LevelMeter.h"

namespace hum {

class TakeLaneBrick : public PolledBrick {
public:
    TakeLaneBrick(BrickHost& host, std::string name, int track, const Bindings& bound)
        : PolledBrick(host, std::move(name)), track_(std::max(1, track)),
          channelsPrefix_(bound(bind::kChannelsPrefix)) {
        history_.assign(kHistory, 0.0f);
    }

    int preferredContentWidth() const override { return 190; }
    int preferredContentHeight(int) const override { return 22; }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().reduced(1);
        auto clock = r.removeFromRight(kClockW);
        auto meter = r.removeFromRight(kMeterW);
        r.removeFromRight(3);
        paintHistory(g, r.toFloat());
        paintMeter(g, meter.reduced(2, 1).toFloat());
        paintClock(g, clock);
    }

    double secondsForTest() const { return seconds_; }
    bool rollingForTest() const { return rolling_; }
    float historyPeakForTest() const {
        return *std::max_element(history_.begin(), history_.end());
    }
    int firstChannelForTest() const { return firstChannel(); }
    int channelsForTest() const { return channels(); }
    int trackForTest() const { return track_; }

private:
    static constexpr int kHistory = 96;
    static constexpr int kMeterW = 26;
    static constexpr int kClockW = 44;
    static constexpr double kFloorDb = -60.0;

    void poll() override {
        const bool wasRolling = rolling_;
        rolling_ = host_.files().isRecorderActive(name_);
        const double now = juce::Time::getMillisecondCounterHiRes();
        if (rolling_ && !wasRolling) {
            std::fill(history_.begin(), history_.end(), 0.0f);
            startedMs_ = now;
        }
        if (rolling_) seconds_ = (now - startedMs_) / 1000.0;

        float lv[LevelMeter::kMax];
        const int have = host_.nodeMeter(name_, lv, LevelMeter::kMax);
        const int from = firstChannel();
        const int span = std::max(1, channels());
        float loudest = 0.0f;
        for (int c = from; c < from + span && c < have; ++c) loudest = std::max(loudest, lv[c]);
        level_ = loudest;

        if (rolling_) {
            history_.erase(history_.begin());
            history_.push_back(loudest);
        }
        repaint();
    }

    int channels() const {
        return (int) std::lround(host_.liveParamValue(
            name_, channelsPrefix_ + std::to_string(track_)));
    }

    int firstChannel() const {
        int at = 0;
        for (int t = 1; t < track_; ++t)
            at += (int) std::lround(host_.liveParamValue(
                name_, channelsPrefix_ + std::to_string(t)));
        return at;
    }

    static float bar(float linear) {
        if (linear <= 0.0f) return 0.0f;
        const double db = 20.0 * std::log10((double) linear);
        return (float) juce::jlimit(0.0, 1.0, 1.0 - db / kFloorDb);
    }

    void paintHistory(juce::Graphics& g, juce::Rectangle<float> r) {
        g.setColour(Palette::background.darker(0.25f));
        g.fillRoundedRectangle(r, 3.0f);
        const auto inner = r.reduced(2.0f);
        const float step = inner.getWidth() / (float) kHistory;
        const float mid = inner.getCentreY();
        g.setColour(rolling_ ? Palette::accent : Palette::accentDim.withAlpha(alpha::mid));
        for (int i = 0; i < kHistory; ++i) {
            const float h = bar(history_[(size_t) i]) * inner.getHeight() * 0.5f;
            if (h <= 0.0f) continue;
            g.fillRect(inner.getX() + (float) i * step, mid - h, std::max(1.0f, step - 0.5f),
                       h * 2.0f);
        }
        g.setColour(Palette::border.withAlpha(alpha::mist));
        g.drawHorizontalLine((int) mid, inner.getX(), inner.getRight());
    }

    void paintMeter(juce::Graphics& g, juce::Rectangle<float> r) {
        g.setColour(Palette::background.darker(0.25f));
        g.fillRoundedRectangle(r, 2.0f);
        const auto inner = r.reduced(1.0f);
        const float lit = bar(level_) * inner.getWidth();
        if (lit <= 0.0f) return;
        g.setColour(level_ >= 1.0f ? ink::state::danger
                                   : level_ > 0.7f ? ink::state::caution : ink::state::ok);
        g.fillRect(inner.withWidth(lit));
    }

    void paintClock(juce::Graphics& g, juce::Rectangle<int> r) {
        const int whole = (int) seconds_;
        g.setColour(rolling_ ? Palette::text : Palette::textDim);
        g.setFont(juce::FontOptions(11.0f));
        g.drawText(juce::String(whole / 60) + ":"
                       + juce::String(whole % 60).paddedLeft('0', 2),
                   r, juce::Justification::centredRight);
    }

    int track_ = 1;
    std::string channelsPrefix_;
    std::vector<float> history_;
    float level_ = 0.0f;
    double seconds_ = 0.0, startedMs_ = 0.0;
    bool rolling_ = false;
};

}
