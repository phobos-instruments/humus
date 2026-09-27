// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/inputs/KnobGridModel.h"
#include "gui/editor/AutomateMenu.h"
#include "gui/style/Colours.h"
#include "gui/editor/OrganismEditor.h"
#include "gui/editor/ParamReset.h"
#include "gui/host/BrickHost.h"
#include "gui/style/LookAndFeel.h"
#include "gui/editor/ParamSlider.h"
#include "gui/bricks/PolledBrick.h"
#include "gui/common/Localisation.h"

namespace hum {

class KnobGridBrick : public PolledBrick {
public:
    KnobGridBrick(BrickHost& host, std::string organism, std::string prefix,
                  int rows, std::vector<std::string> fields)
        : PolledBrick(host, organism), grid_(host, organism, prefix, rows, std::move(fields)) {
        for (int r = 0; r < grid_.rows(); ++r) {
            auto t = std::make_unique<juce::TextButton>(prefix + std::to_string(r + 1));
            t->setClickingTogglesState(true);
            t->setTooltip(tr("knob-grid.mute-this-operator", "Mute this operator"));
            t->setColour(juce::TextButton::buttonOnColourId, Palette::accent.withAlpha(alpha::muted));
            t->setToggleState(grid_.rowOn(r), juce::dontSendNotification);
            auto* tp = t.get();
            t->onClick = [this, tp, r] { grid_.setRow(r, tp->getToggleState()); };
            addAndMakeVisible(*t);
            rowToggles_.push_back(std::move(t));
        }
        for (const auto& spec : grid_.knobs()) {
            auto k = std::make_unique<ParamSlider>(juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox);
            k->paramLabel = juce::String::fromUTF8(spec.field.c_str());
            k->setRange(spec.lo, spec.hi, 0.0);
            k->setNumDecimalPlacesToDisplay(spec.whole ? 0 : 2);
            k->setUnit(spec.unit);
            k->setPopupDisplayEnabled(true, true, this);
            k->setValue(grid_.value(spec.param), juce::dontSendNotification);
            enableDoubleClickReset(*k, host_, name_, spec.param);
            auto* kp = k.get();
            const auto node = name_;
            const auto param = spec.param;
            k->onValueChange = [this, kp, node, param] { host_.editParam(node, param, kp->getValue()); };
            k->onDragStart = [this, node, param] { host_.beginParamDrag(node, param); };
            k->onDragEnd = [this] { host_.endParamDrag(); };
            kp->setParamId(node, param);
            kp->onPopup = [this, node, param](juce::Point<int> at) { showAutomateMenu(host_, node, param, at, nullptr); };
            addAndMakeVisible(*k);
            knobs_.push_back(std::move(k));
        }
    }

    void reloadValues() override {
        for (size_t i = 0; i < knobs_.size(); ++i)
            if (!knobs_[i]->isMouseButtonDown())
                knobs_[i]->setValue(grid_.value(grid_.knobs()[i].param), juce::dontSendNotification);
        const auto columns = grid_.fields().size();
        for (size_t r = 0; r < rowToggles_.size(); ++r) {
            const bool unavailable = grid_.rowUnavailable((int) r);
            rowToggles_[r]->setEnabled(!unavailable);
            rowToggles_[r]->setToggleState(!unavailable && grid_.rowOn((int) r), juce::dontSendNotification);
            for (size_t c = 0; c < columns; ++c) {
                const auto i = r * columns + c;
                if (i < knobs_.size()) knobs_[i]->setEnabled(!unavailable);
            }
        }
    }
    void refreshAutomatedValues() override { reloadValues(); }

    void poll() override {}

    int preferredContentWidth() const override { return grid_.preferredWidth(); }
    int preferredContentHeight(int) const override { return grid_.preferredHeight(); }

    void paint(juce::Graphics& g) override {
        const auto m = grid_.metrics(getWidth(), getHeight());
        const auto& fields = grid_.fields();
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(juce::jmax(6.0f, 9.5f * (float) m.k)));
        for (size_t c = 0; c < fields.size(); ++c)
            g.drawText(juce::String::fromUTF8(fields[c].c_str()),
                       input::KnobGridModel::cellX((int) c, m), 0, m.cell, m.header - 2,
                       juce::Justification::centred, false);
        g.setColour(Palette::border.withAlpha(alpha::dim));
        g.drawHorizontalLine(m.header - 1, 0.0f, (float) getWidth());
        for (int r = 1; r < grid_.rows(); ++r)
            g.drawHorizontalLine(m.header + r * m.cell, (float) m.labelW, (float) getWidth());
    }

    void resized() override {
        const auto m = grid_.metrics(getWidth(), getHeight());
        const int toggleH = juce::jmax(6, (int) std::lround(18.0 * m.k));
        for (size_t r = 0; r < rowToggles_.size(); ++r)
            rowToggles_[r]->setBounds(0,
                                      m.header + (int) r * m.cell + (m.cell - toggleH) / 2,
                                      juce::jmax(1, m.labelW - 2), toggleH);
        const int pad = juce::jmax(1, (int) std::lround(3.0 * m.k));
        const int top = juce::jmax(1, (int) std::lround(2.0 * m.k));
        const int knob = juce::jmax(1, m.cell - 2 * pad);
        const auto columns = grid_.fields().size();
        for (int r = 0; r < grid_.rows(); ++r)
            for (size_t c = 0; c < columns; ++c) {
                const auto i = (size_t) r * columns + c;
                if (i >= knobs_.size()) break;
                knobs_[i]->setBounds(input::KnobGridModel::cellX((int) c, m) + pad,
                                     m.header + r * m.cell + top, knob, knob);
            }
    }

private:
    input::KnobGridModel grid_;
    std::vector<std::unique_ptr<ParamSlider>> knobs_;
    std::vector<std::unique_ptr<juce::TextButton>> rowToggles_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KnobGridBrick)
};

}
