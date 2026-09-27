// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

namespace hum {

class CardStack : public juce::Component, private juce::Timer {
public:
    static constexpr int kGap = 8;
    static constexpr int kSlide = 16;
    static constexpr int kFrameHz = 60;
    static constexpr float kEnterSeconds = 0.22f;
    static constexpr float kLeaveSeconds = 0.18f;
    static constexpr float kSettle = 0.35f;

    CardStack() { setInterceptsMouseClicks(false, true); }

    void push(std::unique_ptr<juce::Component> card) {
        card->setAlpha(0.0f);
        addAndMakeVisible(*card);
        slots_.push_back({std::move(card)});
        animate();
    }

    void remove(juce::Component* card) {
        const auto it = std::find_if(slots_.begin(), slots_.end(),
                                     [card](const Slot& s) { return s.card.get() == card; });
        if (it == slots_.end() || it->leaving) return;
        it->leaving = true;
        it->card->setInterceptsMouseClicks(false, false);
        animate();
    }

    int count() const {
        return (int) std::count_if(slots_.begin(), slots_.end(), [](const Slot& s) { return !s.leaving; });
    }

    void childBoundsChanged(juce::Component*) override {
        if (!fitting_) animate();
    }

    void placeAbove(juce::Rectangle<int> parentArea, int bottom) {
        anchor_ = {parentArea.getRight(), bottom};
        fit();
    }

private:
    struct Slot {
        std::unique_ptr<juce::Component> card;
        float shown = 0.0f;
        float fromBottom = -1.0f;
        bool leaving = false;
    };

    static float eased(float t) { return 1.0f - (1.0f - t) * (1.0f - t); }

    void animate() {
        if (!fit()) startTimerHz(kFrameHz);
    }

    void timerCallback() override {
        const float enter = 1.0f / (kEnterSeconds * (float) kFrameHz);
        const float leave = 1.0f / (kLeaveSeconds * (float) kFrameHz);
        for (auto& s : slots_)
            s.shown = s.leaving ? std::max(0.0f, s.shown - leave) : std::min(1.0f, s.shown + enter);
        for (auto it = slots_.begin(); it != slots_.end();) {
            if (!it->leaving || it->shown > 0.0f) { ++it; continue; }
            auto* gone = it->card.release();
            it = slots_.erase(it);
            gone->setVisible(false);
            removeChildComponent(gone);
            juce::MessageManager::callAsync([gone] { delete gone; });
        }
        if (fit()) stopTimer();
    }

    bool fit() {
        const juce::ScopedValueSetter<bool> guard(fitting_, true);
        int w = 0, h = 0;
        for (const auto& s : slots_) {
            w = std::max(w, s.card->getWidth());
            h += s.card->getHeight() + (h > 0 ? kGap : 0);
        }
        setBounds(anchor_.x - w, anchor_.y - h, w + kSlide, h);
        bool settled = true;
        float below = 0.0f;
        for (auto& s : slots_) {
            if (s.fromBottom < 0.0f) s.fromBottom = below;
            s.fromBottom += (below - s.fromBottom) * kSettle;
            if (std::abs(below - s.fromBottom) < 0.5f) s.fromBottom = below;
            const float t = eased(s.shown);
            const int x = w - s.card->getWidth() + juce::roundToInt((1.0f - t) * (float) kSlide);
            const int y = h - juce::roundToInt(s.fromBottom) - s.card->getHeight();
            s.card->setTopLeftPosition(x, y);
            s.card->setAlpha(t);
            settled = settled && s.fromBottom == below && s.shown == (s.leaving ? 0.0f : 1.0f);
            below += (float) (s.card->getHeight() + kGap);
        }
        setVisible(!slots_.empty());
        return settled;
    }

    std::vector<Slot> slots_;
    juce::Point<int> anchor_;
    bool fitting_ = false;
};

}
