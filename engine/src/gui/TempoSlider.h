#pragma once
#include <cmath>
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/LookAndFeel.h"

namespace hum {

class TempoSlider : public juce::Slider {
public:
    static constexpr int kNumberW = 86;

    std::function<void(juce::Point<int>)> onPopup;

    void resized() override {
        juce::Slider::resized();
        numberBox_ = nullptr;
        for (int i = 0; i < getNumChildComponents(); ++i) {
            auto* c = getChildComponent(i);
            c->removeMouseListener(this);
            c->addMouseListener(this, false);
            if (numberBox_ == nullptr) {
                numberBox_ = dynamic_cast<juce::Label*>(c);
                if (numberBox_ != nullptr)
                    numberBox_->setFont(juce::Font(juce::FontOptions(13.0f).withStyle("Bold")));
            }
            if (auto* tb = dynamic_cast<juce::TextButton*>(c)) {
                tb->setColour(juce::TextButton::buttonColourId, Palette::panel);
                tb->setColour(juce::TextButton::textColourOffId, Palette::textDim);
                tb->setColour(juce::TextButton::textColourOnId, Palette::text);
                tb->getProperties().set("flatPill", true);
            }
        }
    }

    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu() && onPopup) { onPopup(e.getScreenPosition()); return; }
        if (e.originalComponent == numberBox_ && numberBox_ != nullptr) {
            dragFrom_ = getValue();
            dragging_ = false;
            return;
        }
        if (e.originalComponent != this) return;
        juce::Slider::mouseDown(e);
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (e.originalComponent == numberBox_ && numberBox_ != nullptr) {
            if (!dragging_ && std::abs(e.getDistanceFromDragStartY()) > 2) {
                dragging_ = true;
                numberBox_->setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
            }
            if (!dragging_) return;
            const double perPx = e.mods.isShiftDown() ? 0.05 : 0.4;
            setValue(dragFrom_ - e.getDistanceFromDragStartY() * perPx);
            return;
        }
        if (e.originalComponent != this) return;
        juce::Slider::mouseDrag(e);
    }

    void mouseUp(const juce::MouseEvent& e) override {
        if (e.originalComponent == numberBox_ && numberBox_ != nullptr) {
            dragging_ = false;
            numberBox_->setMouseCursor(juce::MouseCursor::NormalCursor);
            return;
        }
        if (e.originalComponent != this) return;
        juce::Slider::mouseUp(e);
    }

    bool userDragging() const { return dragging_ || isMouseButtonDown(true); }

    void setExternallyControlled(bool b) {
        if (externallyControlled_ == b) return;
        externallyControlled_ = b;
        repaint();
    }

    void paintOverChildren(juce::Graphics& g) override {
        if (!externallyControlled_) return;
        g.setColour(Palette::accent.withAlpha(0.55f));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 6.0f, 1.2f);
    }

private:
    juce::Label* numberBox_ = nullptr;
    bool externallyControlled_ = false;
    double dragFrom_ = 120.0;
    bool dragging_ = false;
};

}
