// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <array>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/bricks/PolledBrick.h"
#include "gui/host/BrickHost.h"
#include "gui/style/Colours.h"
#include "gui/style/LookAndFeel.h"
#include "hum/caps/Midi.h"

namespace hum {

class KeyedTapeBrick : public PolledBrick {
public:
    static constexpr int kMaxRuns = 4096;
    static constexpr float kUnitPx = 4.0f;

    KeyedTapeBrick(BrickHost& host, std::string organism) : PolledBrick(host, std::move(organism), 2) { poll(); }

    void reloadValues() override { poll(); }
    int preferredContentWidth() const override { return 280; }
    int preferredContentHeight(int) const override { return 20; }

    int runCountForTest() const { return (int) units_.size(); }
    int nowForTest() const { return now_; }

    void paint(juce::Graphics& g) override {
        const auto area = getLocalBounds().toFloat();
        g.setColour(Palette::background.withAlpha(alpha::strong));
        g.fillRoundedRectangle(area, 3.0f);
        if (units_.empty()) return;
        const float cy = area.getCentreY(), dot = std::min(area.getHeight() * 0.36f, kUnitPx * 1.3f);
        float x = area.getX() + 6.0f - scrollFor(area.getWidth() - 12.0f);
        for (size_t i = 0; i < units_.size(); ++i) {
            const float w = (float) units_[i] * kUnitPx;
            if (on_[i] && x + w > area.getX() && x < area.getRight()) {
                const bool lit = (int) i == now_;
                g.setColour(lit ? Palette::accent : Palette::textDim.withAlpha(alpha::strong));
                if (units_[i] <= 1) g.fillEllipse(x + w * 0.5f - dot * 0.5f, cy - dot * 0.5f, dot, dot);
                else g.fillRoundedRectangle(x, cy - dot * 0.5f, w, dot, dot * 0.5f);
            }
            x += w;
        }
    }

private:
    float scrollFor(float visible) const {
        if (now_ < 0) return 0.0f;
        float before = 0.0f, total = 0.0f;
        for (size_t i = 0; i < units_.size(); ++i) {
            if ((int) i < now_) before += (float) units_[i] * kUnitPx;
            total += (float) units_[i] * kUnitPx;
        }
        if (total <= visible) return 0.0f;
        return std::clamp(before - visible * 0.3f, 0.0f, total - visible);
    }

    void poll() override {
        const auto* tape = live<KeyedTape>();
        std::vector<int> units;
        std::vector<char> on;
        int now = -1;
        if (tape != nullptr) {
            const int n = tape->tapeRuns(unitBuf_.data(), onBuf_.data(), kMaxRuns);
            units.assign(unitBuf_.begin(), unitBuf_.begin() + n);
            on.assign(onBuf_.begin(), onBuf_.begin() + n);
            now = tape->tapeRunNow();
        }
        if (units == units_ && on == on_ && now == now_) return;
        units_ = std::move(units);
        on_ = std::move(on);
        now_ = now;
        repaint();
    }

    std::array<int, kMaxRuns> unitBuf_{};
    std::array<bool, kMaxRuns> onBuf_{};
    std::vector<int> units_;
    std::vector<char> on_;
    int now_ = -1;
};

}
