// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/ScrollModel.h"
#include "gui/style/HandCursors.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

class PanelScroller : public juce::Component {
public:
    static constexpr int kBarWidth = 12;
    static constexpr int kBarInset = 10;
    static constexpr int kGrabPad = 6;

    PanelScroller() {
        bar_.owner = this;
        bar_.setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        addChildComponent(bar_);
    }

    void setContent(juce::Component& content) {
        content_ = &content;
        addAndMakeVisible(content, 0);
        place();
    }
    void setContentHeight(int height) {
        model_.resize(height, getHeight());
        place();
    }
    int contentHeight() const { return model_.content; }
    int position() const { return model_.position(); }
    bool scrollable() const { return model_.scrollable(); }

    juce::Component& barForTest() { return bar_; }

    bool wheel(const juce::MouseWheelDetails& w) {
        if (!model_.moveBy(scroll::wheelPixels(w.deltaY, w.isReversed))) return false;
        place();
        return true;
    }

    static bool takesWheel(juce::Component& inside, const juce::MouseEvent& e, const juce::MouseWheelDetails& w) {
        if (e.mods.isAltDown() || e.mods.isCommandDown() || e.mods.isCtrlDown() || e.mods.isShiftDown()) return false;
        auto* scroller = inside.findParentComponentOfClass<PanelScroller>();
        return scroller != nullptr && scroller->wheel(w);
    }

    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override {
        if (!wheel(w)) juce::Component::mouseWheelMove(e, w);
    }
    void resized() override {
        model_.resize(model_.content, getHeight());
        place();
    }

private:
    struct Bar : juce::Component {
        PanelScroller* owner = nullptr;
        bool hover = false;
        int grabOffset = -1;

        int track() const { return getHeight(); }
        void paint(juce::Graphics& g) override {
            const auto r = getLocalBounds().reduced(kGrabPad, 0).toFloat();
            const float radius = r.getWidth() * 0.5f;
            g.setColour(Palette::border);
            g.fillRoundedRectangle(r, radius);
            const auto t = owner->model_.thumb(track());
            g.setColour(hover || grabOffset >= 0 ? Palette::accent.brighter(0.25f) : Palette::accent);
            g.fillRoundedRectangle(r.withY((float) t.top).withHeight((float) t.height), radius);
        }
        void mouseEnter(const juce::MouseEvent&) override { hover = true; repaint(); }
        void mouseExit(const juce::MouseEvent&) override { hover = false; repaint(); }
        void mouseDown(const juce::MouseEvent& e) override {
            const auto t = owner->model_.thumb(track());
            if (e.y >= t.top && e.y < t.top + t.height) {
                grabOffset = e.y - t.top;
                showCursor(grabbingHandCursor());
                return;
            }
            const double page = owner->model_.view * (e.y < t.top ? -1.0 : 1.0);
            if (owner->model_.moveBy(page)) owner->place();
        }
        void mouseDrag(const juce::MouseEvent& e) override {
            if (grabOffset < 0) return;
            if (owner->model_.moveTo(owner->model_.offsetForThumbTop(e.y - grabOffset, track()))) owner->place();
        }
        void mouseUp(const juce::MouseEvent&) override {
            grabOffset = -1;
            showCursor(juce::MouseCursor::DraggingHandCursor);
            repaint();
        }
        void showCursor(const juce::MouseCursor& cursor) {
            setMouseCursor(cursor);
            juce::Desktop::getInstance().getMainMouseSource().forceMouseCursorUpdate();
        }
    };

    void place() {
        if (content_ != nullptr) content_->setBounds(0, -model_.position(), getWidth(), model_.content);
        bar_.setVisible(model_.scrollable());
        bar_.setBounds(getWidth() - kBarWidth - kBarInset - kGrabPad, kBarInset, kBarWidth + 2 * kGrabPad,
                       juce::jmax(0, getHeight() - 2 * kBarInset));
        bar_.toFront(false);
        bar_.repaint();
    }

    scroll::Model model_;
    juce::Component* content_ = nullptr;
    Bar bar_;
};

}
