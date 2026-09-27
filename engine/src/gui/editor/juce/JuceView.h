// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/juce/JuceGeometry.h"
#include "gui/editor/views/BrickView.h"

namespace hum {

inline juce::String viewText(const std::string& text) {
    return juce::String::fromUTF8(text.c_str());
}

template <class Widget, class Contract>
class JuceView : public Widget, public Contract {
public:
    using Widget::Widget;

    void setViewBounds(Rect bounds) override { this->setBounds(toJuce(bounds)); }
    void setViewVisible(bool visible) override { this->setVisible(visible); }
    bool viewVisible() const override { return this->isVisible(); }
    void setViewFade(float alpha, bool enabled) override {
        this->setAlpha(alpha);
        this->setEnabled(enabled);
    }
    float viewAlpha() const override { return this->getAlpha(); }
    bool viewEnabled() const override { return this->isEnabled(); }
    void setViewTooltip(const std::string& tip) override { this->setTooltip(viewText(tip)); }
};

class JucePartsOwner {
public:
    virtual ~JucePartsOwner() = default;
    const std::vector<juce::Component*>& juceParts() const { return parts_; }

protected:
    std::vector<juce::Component*> parts_;
};

template <class View>
std::vector<juce::Component*> juceWidgetsOf(View* view) {
    if (auto* widget = dynamic_cast<juce::Component*>(view)) return {widget};
    if (auto* group = dynamic_cast<JucePartsOwner*>(view)) return group->juceParts();
    return {};
}

template <class Contract>
class JuceGroupView : public Contract, public JucePartsOwner {
public:
    void setViewVisible(bool visible) override {
        for (auto* part : parts_) part->setVisible(visible);
    }
    bool viewVisible() const override { return !parts_.empty() && parts_.front()->isVisible(); }
    void setViewFade(float alpha, bool enabled) override {
        for (auto* part : parts_) {
            part->setAlpha(alpha);
            part->setEnabled(enabled);
        }
    }
    float viewAlpha() const override { return parts_.empty() ? 1.0f : parts_.front()->getAlpha(); }
    bool viewEnabled() const override { return parts_.empty() || parts_.front()->isEnabled(); }
};

}
