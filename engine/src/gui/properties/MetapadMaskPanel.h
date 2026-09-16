// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <memory>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/host/PropertiesHost.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

class MetapadMaskPanel : public juce::Component {
public:
    explicit MetapadMaskPanel(PropertiesHost& host) : host_(host) {
        addAndMakeVisible(vp_);
        vp_.setViewedComponent(&content_, false);
        rebuild();
    }

    void resized() override {
        vp_.setBounds(getLocalBounds());
        layout();
    }

    void rebuild() {
        rows_.clear();
        content_.removeAllChildren();
        const auto& ms = host_.model().metapad;
        for (const auto& c : host_.model().organisms) {
            std::vector<const MetapadMaskEntry*> entries;
            for (const auto& e : ms.mask)
                if (e.organismName == c.name) entries.push_back(&e);
            if (entries.empty()) continue;
            rows_.push_back(std::make_unique<GroupRow>(*this, c.name));
            for (const auto* e : entries)
                rows_.push_back(std::make_unique<ParamRow>(*this, c.name, e->propertyIndex,
                                                           paramName(c.name, e->propertyIndex), e->restore));
        }
        for (auto& r : rows_) content_.addAndMakeVisible(*r);
        layout();
    }

private:
    void setGroup(const std::string& organism, bool restore) {
        std::vector<int> indices;
        for (const auto& e : host_.model().metapad.mask)
            if (e.organismName == organism) indices.push_back(e.propertyIndex);
        for (int idx : indices) host_.metapad().setMask(organism, idx, restore);
        rebuild();
    }

    juce::String paramName(const std::string& organism, int idx) const {
        if (auto* cm = host_.model().byName(organism))
            for (const auto& p : cm->properties)
                if (p.index == idx) return juce::String(p.name);
        return juce::String(idx);
    }

    void styleBtn(juce::Button& b) {
        b.setColour(juce::TextButton::buttonColourId, Palette::panelLight);
        b.setColour(juce::TextButton::buttonOnColourId, Palette::accent);
        b.setColour(juce::TextButton::textColourOffId, Palette::text);
        b.setColour(juce::TextButton::textColourOnId, Palette::background);
    }

    struct GroupRow : juce::Component {
        GroupRow(MetapadMaskPanel& p, std::string c) : panel(p), organism(std::move(c)) {
            lbl.setText(organism, juce::dontSendNotification);
            lbl.setColour(juce::Label::textColourId, Palette::accent);
            lbl.setFont(juce::FontOptions(12.0f).withStyle("Bold"));
            addAndMakeVisible(lbl);
            for (auto* b : {&all, &none}) { addAndMakeVisible(*b); panel.styleBtn(*b); }
            all.setButtonText("all"); none.setButtonText("none");
            all.onClick  = [this] { panel.setGroup(organism, true); };
            none.onClick = [this] { panel.setGroup(organism, false); };
        }
        void resized() override {
            auto r = getLocalBounds();
            auto btns = r.removeFromRight(84);
            all.setBounds(btns.removeFromLeft(42).reduced(1));
            none.setBounds(btns.removeFromLeft(42).reduced(1));
            lbl.setBounds(r.reduced(2, 0));
        }
        MetapadMaskPanel& panel; std::string organism;
        juce::Label lbl; juce::TextButton all, none;
    };

    struct ParamRow : juce::Component {
        ParamRow(MetapadMaskPanel& p, std::string c, int idx, juce::String name, bool on)
            : panel(p), organism(std::move(c)), propertyIndex(idx) {
            tog.setButtonText(name);
            tog.setToggleState(on, juce::dontSendNotification);
            tog.setColour(juce::ToggleButton::textColourId, Palette::text);
            tog.setColour(juce::ToggleButton::tickColourId, Palette::accent);
            tog.onClick = [this] {
                panel.host_.metapad().setMask(organism, propertyIndex, tog.getToggleState());
            };
            addAndMakeVisible(tog);
        }
        void resized() override { tog.setBounds(getLocalBounds().withTrimmedLeft(16)); }
        MetapadMaskPanel& panel; std::string organism; int propertyIndex;
        juce::ToggleButton tog;
    };

    void layout() {
        const int rowH = 22;
        content_.setSize(juce::jmax(180, vp_.getWidth() - 4), juce::jmax(1, (int) rows_.size() * rowH));
        for (size_t i = 0; i < rows_.size(); ++i)
            rows_[i]->setBounds(0, (int) i * rowH, content_.getWidth(), rowH);
    }

    PropertiesHost& host_;
    juce::Viewport vp_;
    juce::Component content_;
    std::vector<std::unique_ptr<juce::Component>> rows_;
};

}
