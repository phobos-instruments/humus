// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <utility>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/bricks/StepGridReadout.h"
#include "gui/editor/grids/StepGridModel.h"
#include "gui/host/BrickHost.h"
#include "gui/style/Colours.h"

namespace hum {

class StepChanceView {
public:
    StepChanceView(BrickHost& host, std::string organism, std::string viewParam, const grids::StepGridModel& grid)
        : host_(host), organism_(std::move(organism)), viewParam_(std::move(viewParam)), grid_(grid) {}

    bool available() const { return grid_.hasChances(); }

    bool on() const {
        if (!available()) return false;
        if (viewParam_.empty()) return local_;
        return stepgrid::viewOf(host_.liveParamValue(organism_, viewParam_)) == stepgrid::kChanceView;
    }

    void toggle() {
        if (viewParam_.empty()) local_ = !local_;
        else host_.editParam(organism_, viewParam_, on() ? stepgrid::kStepsView : stepgrid::kChanceView);
        changed();
    }

    bool changed() {
        const bool now = on();
        unsigned sum = 0;
        for (int col = 0; available() && col < grid_.stepCount(); ++col)
            sum = sum * 31u + (unsigned) grid_.chanceAtColumn(col);
        if (now == shown_ && sum == sum_) return false;
        shown_ = now;
        sum_ = sum;
        return true;
    }

    void setDragging(bool dragging) { dragging_ = dragging; }
    bool dragging() const { return dragging_; }

    static juce::Rectangle<float> key(int width, float top) {
        return {(float) (width - stepgrid::kChanceKeyW), top, (float) stepgrid::kChanceKeyW, 18.0f};
    }

    void paint(juce::Graphics& g, const std::vector<bool>& plays, float gridWidth, float height, juce::Colour fam) const {
        const int n = grid_.stepCount();
        if (n <= 0) return;
        const float cw = gridWidth / (float) n;
        const bool editing = on() || dragging_;
        if (editing) g.fillAll(Palette::background.withAlpha(alpha::mid));
        for (int col = 0; col < n; ++col) {
            const juce::Rectangle<float> column(col * cw + 2.0f, 4.0f, cw - 4.0f, height - 8.0f);
            const int chance = grid_.chanceAtColumn(col);
            if (editing) {
                stepgrid::paintChanceBar(g, column, chance, fam);
            } else if (col < (int) plays.size() && plays[(size_t) col]) {
                stepgrid::paintChanceFade(g, column, chance, Palette::panel.darker(0.06f));
                stepgrid::paintChanceTag(g, column.withHeight(12.0f), chance, Palette::textDim);
            }
        }
        const int drag = grid_.dragColumn();
        if (dragging_ && drag >= 0)
            stepgrid::paintReadout(g, stepgrid::percentText(grid_.chanceAtColumn(drag)), drag * cw + cw * 0.5f,
                                   height * 0.5f, gridWidth, fam);
    }

private:
    BrickHost& host_;
    std::string organism_, viewParam_;
    const grids::StepGridModel& grid_;
    bool local_ = false, shown_ = false, dragging_ = false;
    unsigned sum_ = 0;
};

}
