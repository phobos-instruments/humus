// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <memory>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

namespace hum {

class CardStack : public juce::Component {
public:
    static constexpr int kGap = 8;

    CardStack() { setInterceptsMouseClicks(false, true); }

    void push(std::unique_ptr<juce::Component> card) {
        addAndMakeVisible(*card);
        cards_.push_back(std::move(card));
        fit();
    }

    void remove(juce::Component* card) {
        const auto it = std::find_if(cards_.begin(), cards_.end(),
                                     [card](const auto& c) { return c.get() == card; });
        if (it == cards_.end()) return;
        auto* gone = it->release();
        cards_.erase(it);
        gone->setVisible(false);
        removeChildComponent(gone);
        juce::MessageManager::callAsync([gone] { delete gone; });
        fit();
    }

    int count() const { return (int) cards_.size(); }

    void placeAbove(juce::Rectangle<int> parentArea, int bottom) {
        anchor_ = {parentArea.getRight(), bottom};
        fit();
    }

private:
    void fit() {
        int w = 0, h = 0;
        for (const auto& c : cards_) {
            w = std::max(w, c->getWidth());
            h += c->getHeight() + (h > 0 ? kGap : 0);
        }
        setBounds(anchor_.x - w, anchor_.y - h, w, h);
        int y = h;
        for (const auto& c : cards_) {
            y -= c->getHeight();
            c->setTopLeftPosition(w - c->getWidth(), y);
            y -= kGap;
        }
        setVisible(!cards_.empty());
    }

    std::vector<std::unique_ptr<juce::Component>> cards_;
    juce::Point<int> anchor_;
};

}
