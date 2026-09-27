// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <utility>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/bricks/PolledBrick.h"
#include "gui/common/Localisation.h"
#include "gui/style/Colours.h"
#include "gui/style/StatusDot.h"

namespace hum {

class PictureLamp : public PolledBrick {
public:
    static constexpr int kQuietPolls = 8;

    PictureLamp(BrickHost& host, std::string organism) : PolledBrick(host, std::move(organism)) {}

    void reloadValues() override { repaint(); }
    void refreshAutomatedValues() override {}
    int preferredContentWidth() const override { return 46; }
    int preferredContentHeight(int) const override { return 22; }

    void paint(juce::Graphics& g) override {
        StatusDot lamp{tr("picture-lamp.vid", "VID"), tint(), false};
        lamp.paint(g, getLocalBounds().withSizeKeepingCentre(
                          juce::jmin(getWidth(), lamp.width()), StatusDot::kHeight));
    }

private:
    void poll() override {
        const auto from = source();
        if (from.empty()) {
            if (quiet_ != kQuietPolls || corded_) {
                quiet_ = kQuietPolls;
                corded_ = false;
                repaint();
            }
            return;
        }
        const unsigned now = host_.nodePictureGeneration(from);
        const bool moved = now != seen_ || !corded_;
        seen_ = now;
        corded_ = true;
        const int was = quiet_;
        quiet_ = moved && now != 0 ? 0 : juce::jmin(kQuietPolls, quiet_ + 1);
        if (quiet_ != was) repaint();
    }

    std::string source() const {
        for (const auto& c : host_.model().videoConnections)
            if (c.dst == name_ && c.dstInlet == 0) return c.src;
        return {};
    }

    juce::Colour tint() const {
        if (!corded_) return Palette::border;
        return quiet_ < kQuietPolls ? ink::state::ok : ink::state::armed;
    }

    unsigned seen_ = 0;
    int quiet_ = kQuietPolls;
    bool corded_ = false;
};

}
