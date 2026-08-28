#pragma once
#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/ParamSchema.h"
#include "core/ParamUnit.h"
#include "gui/OrganismEditor.h"
#include "gui/ParamReset.h"
#include "gui/EngineHost.h"
#include "gui/LookAndFeel.h"
#include "gui/ParamSlider.h"
#include "gui/PolledBrick.h"

namespace hum {

class KnobGridBrick : public PolledBrick {
public:
    KnobGridBrick(EngineHost& host, std::string organism, std::string prefix,
                  int rows, std::vector<std::string> fields)
        : PolledBrick(host, std::move(organism)), prefix_(std::move(prefix)),
          rows_(rows), fields_(std::move(fields)) {
        const auto* cm = host_.model().byName(name_);
        const auto& schema = schemaFor(cm != nullptr ? cm->classRaw : std::string{});
        for (int r = 1; r <= rows_; ++r) {
            const auto param = prefix_ + std::to_string(r) + "_On";
            auto t = std::make_unique<juce::TextButton>(prefix_ + std::to_string(r));
            t->setClickingTogglesState(true);
            t->setTooltip("Mute this operator");
            t->setColour(juce::TextButton::buttonOnColourId, Palette::accent.withAlpha(0.35f));
            t->setToggleState(host_.liveParamValue(name_, param) >= 0.5,
                              juce::dontSendNotification);
            auto* tp = t.get();
            const auto node = name_;
            t->onClick = [this, tp, node, param] {
                host_.editParam(node, param, tp->getToggleState() ? 1.0 : 0.0);
            };
            addAndMakeVisible(*t);
            rowToggles_.push_back(std::move(t));
            onParams_.push_back(param);
        }
        for (int r = 1; r <= rows_; ++r)
            for (const auto& field : fields_) {
                const auto param = prefix_ + std::to_string(r) + "_" + field;
                double lo = 0.0, hi = 1.0;
                bool whole = false;
                Unit unit = Unit::None;
                for (const auto& d : schema)
                    if (d.name == param) {
                        lo = d.min; hi = d.max; whole = d.isInt;
                        unit = unitResolve(d.name, d.unit, d.min, d.max);
                        break;
                    }
                auto k = std::make_unique<ParamSlider>(juce::Slider::RotaryVerticalDrag,
                                                       juce::Slider::NoTextBox);
                k->paramLabel = juce::String::fromUTF8(field.c_str());
                k->setRange(lo, hi, 0.0);
                k->setNumDecimalPlacesToDisplay(whole ? 0 : 2);
                k->setUnit(unit);
                k->setPopupDisplayEnabled(true, true, this);
                k->setValue(host_.liveParamValue(name_, param), juce::dontSendNotification);
                enableDoubleClickReset(*k, host_, name_, param);
                auto* kp = k.get();
                const auto node = name_;
                k->onValueChange = [this, kp, node, param] {
                    host_.editParam(node, param, kp->getValue());
                };
                k->onDragStart = [this, node, param] { host_.beginParamDrag(node, param); };
                k->onDragEnd = [this] { host_.endParamDrag(); };
                kp->setParamId(node, param);
                kp->onPopup = [this, node, param](juce::Point<int> at) {
                    showAutomateMenu(host_, node, param, at, nullptr);
                };
                addAndMakeVisible(*k);
                knobs_.push_back(std::move(k));
                params_.push_back(param);
            }
    }

    void reloadValues() override {
        for (size_t i = 0; i < knobs_.size(); ++i)
            if (!knobs_[i]->isMouseButtonDown())
                knobs_[i]->setValue(host_.liveParamValue(name_, params_[i]),
                                    juce::dontSendNotification);
        auto* live = dynamic_cast<LiveParamRange*>(host_.liveOrganism(name_));
        for (size_t r = 0; r < rowToggles_.size(); ++r) {
            double lo = 0.0, hi = 0.0;
            const bool unavailable = live != nullptr
                                     && live->liveParamRange(onParams_[r], lo, hi) && hi <= lo;
            rowToggles_[r]->setEnabled(!unavailable);
            rowToggles_[r]->setToggleState(!unavailable
                                               && host_.liveParamValue(name_, onParams_[r]) >= 0.5,
                                           juce::dontSendNotification);
            for (size_t c = 0; c < fields_.size(); ++c) {
                const auto i = r * fields_.size() + c;
                if (i < knobs_.size()) knobs_[i]->setEnabled(!unavailable);
            }
        }
    }
    void refreshAutomatedValues() override { reloadValues(); }

    void poll() override {}

    int preferredContentWidth() const override {
        return kLabelW + (int) fields_.size() * kCell;
    }
    int preferredContentHeight(int) const override { return kHeader + rows_ * kCell; }

    void paint(juce::Graphics& g) override {
        const auto m = metrics();
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(juce::jmax(6.0f, 9.5f * (float) m.k)));
        for (size_t c = 0; c < fields_.size(); ++c)
            g.drawText(juce::String::fromUTF8(fields_[c].c_str()),
                       cellX((int) c, m), 0, m.cell, m.header - 2,
                       juce::Justification::centred, false);
        g.setColour(Palette::border.withAlpha(0.5f));
        g.drawHorizontalLine(m.header - 1, 0.0f, (float) getWidth());
        for (int r = 1; r < rows_; ++r)
            g.drawHorizontalLine(m.header + r * m.cell, (float) m.labelW, (float) getWidth());
    }

    void resized() override {
        const auto m = metrics();
        const int toggleH = juce::jmax(6, (int) std::lround(18.0 * m.k));
        for (size_t r = 0; r < rowToggles_.size(); ++r)
            rowToggles_[r]->setBounds(0,
                                      m.header + (int) r * m.cell + (m.cell - toggleH) / 2,
                                      juce::jmax(1, m.labelW - 2), toggleH);
        const int pad = juce::jmax(1, (int) std::lround(3.0 * m.k));
        const int top = juce::jmax(1, (int) std::lround(2.0 * m.k));
        const int knob = juce::jmax(1, m.cell - 2 * pad);
        for (int r = 0; r < rows_; ++r)
            for (size_t c = 0; c < fields_.size(); ++c) {
                const auto i = (size_t) r * fields_.size() + c;
                if (i >= knobs_.size()) break;
                knobs_[i]->setBounds(cellX((int) c, m) + pad,
                                     m.header + r * m.cell + top, knob, knob);
            }
    }

private:
    static constexpr int kCell = 46;
    static constexpr int kHeader = 14;
    static constexpr int kLabelW = 40;

    struct Metrics { double k; int cell, header, labelW; };
    Metrics metrics() const {
        const double pw = preferredContentWidth();
        const double ph = preferredContentHeight(0);
        const double k = (pw <= 0.0 || ph <= 0.0)
                             ? 1.0
                             : std::min({1.0, getWidth() / pw, getHeight() / ph});
        return {k, juce::jmax(1, (int) std::lround(kCell * k)),
                juce::jmax(1, (int) std::lround(kHeader * k)),
                juce::jmax(1, (int) std::lround(kLabelW * k))};
    }

    int cellX(int col, const Metrics& m) const { return m.labelW + col * m.cell; }

    std::string prefix_;
    int rows_;
    std::vector<std::string> fields_;
    std::vector<std::unique_ptr<ParamSlider>> knobs_;
    std::vector<std::string> params_;
    std::vector<std::unique_ptr<juce::TextButton>> rowToggles_;
    std::vector<std::string> onParams_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KnobGridBrick)
};

}
