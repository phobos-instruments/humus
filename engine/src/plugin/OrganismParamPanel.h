#pragma once
#include <memory>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/ParamSchema.h"
#include "plugin/FxLook.h"
#include "plugin/HumusProcessor.h"
#include "gui/Localisation.h"

namespace hum {

class OrganismParamPanel : public juce::Component {
public:
    explicit OrganismParamPanel(HumusProcessor& p) : proc_(p) {
        title_.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
        title_.setColour(juce::Label::textColourId, fxlook::accent());
        addAndMakeVisible(title_);
        klass_.setFont(juce::FontOptions(11.0f));
        klass_.setColour(juce::Label::textColourId, fxlook::dim());
        addAndMakeVisible(klass_);
        viewport_.setViewedComponent(&content_, false);
        viewport_.setScrollBarsShown(true, false);
        addAndMakeVisible(viewport_);
        setTarget("");
    }

    void setTarget(const std::string& organismName) {
        target_ = organismName;
        rows_.clear();
        content_.removeAllChildren();
        const OrganismModel* cm = proc_.model().byName(target_);
        title_.setText(cm ? juce::String(cm->name) : tr("organism-param.no-organism-selected", "(no organism selected)"),
                       juce::dontSendNotification);
        klass_.setText(cm ? juce::String(cm->displayClass) : tr("organism-param.click-a-box-in-the", "click a box in the patch map"),
                       juce::dontSendNotification);
        if (cm != nullptr) {
            for (const auto& d : schemaFor(cm->displayClass)) {
                if (d.isText || d.isPlainText || d.isTrigger) continue;
                auto row = std::make_unique<Row>();
                row->label.setText(d.name, juce::dontSendNotification);
                row->label.setFont(juce::FontOptions(11.0f));
                row->label.setColour(juce::Label::textColourId, fxlook::text());
                content_.addAndMakeVisible(row->label);
                auto& s = row->slider;
                s.setSliderStyle(juce::Slider::LinearHorizontal);
                s.setTextBoxStyle(juce::Slider::TextBoxRight, false, 52, 16);
                const bool stepped = d.isBool || d.isEnum || d.isInt;
                s.setRange(d.min, std::max(d.min + (stepped ? 1.0 : 1e-9), d.max),
                           stepped ? 1.0 : 0.0);
                s.setValue(proc_.liveParam(target_, d.name, d.def), juce::dontSendNotification);
                fxlook::styleSlider(s);
                const std::string pname = d.name;
                s.onValueChange = [this, pname, &s] {
                    proc_.setLiveParam(target_, pname, s.getValue());
                };
                content_.addAndMakeVisible(s);
                rows_.push_back(std::move(row));
            }
        }
        layoutRows();
    }

    const std::string& target() const { return target_; }

    void refreshValues() {
        const OrganismModel* cm = proc_.model().byName(target_);
        if (cm == nullptr) return;
        int i = 0;
        for (const auto& d : schemaFor(cm->displayClass)) {
            if (d.isText || d.isPlainText || d.isTrigger) continue;
            if (i >= (int) rows_.size()) break;
            auto& s = rows_[(size_t) i]->slider;
            if (!s.isMouseButtonDown())
                s.setValue(proc_.liveParam(target_, d.name, d.def), juce::dontSendNotification);
            ++i;
        }
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(fxlook::panel());
        g.setColour(fxlook::line());
        g.drawRect(getLocalBounds());
    }

    void resized() override {
        auto r = getLocalBounds().reduced(8);
        title_.setBounds(r.removeFromTop(18));
        klass_.setBounds(r.removeFromTop(14));
        r.removeFromTop(4);
        viewport_.setBounds(r);
        layoutRows();
    }

private:
    struct Row {
        juce::Label label;
        juce::Slider slider;
    };

    void layoutRows() {
        constexpr int kRowH = 24;
        const int w = juce::jmax(60, viewport_.getWidth() - viewport_.getScrollBarThickness());
        content_.setSize(w, kRowH * (int) rows_.size());
        int y = 0;
        for (auto& row : rows_) {
            auto rr = juce::Rectangle<int>(0, y, w, kRowH).reduced(0, 2);
            row->label.setBounds(rr.removeFromLeft(juce::jmin(110, w / 3)));
            row->slider.setBounds(rr);
            y += kRowH;
        }
    }

    HumusProcessor& proc_;
    std::string target_;
    juce::Label title_, klass_;
    juce::Viewport viewport_;
    juce::Component content_;
    std::vector<std::unique_ptr<Row>> rows_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrganismParamPanel)
};

}
