// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <memory>
#include <string>
#include <juce_gui_basics/juce_gui_basics.h>
#include "core/packs/ClassString.h"
#include "gui/editor/AutomateMenu.h"
#include "gui/style/Colours.h"
#include "gui/host/PropertiesHost.h"
#include "gui/style/IconButton.h"
#include "gui/style/LookAndFeel.h"
#include "gui/properties/PresetActions.h"
#include "gui/properties/PresetLibrary.h"
#include "gui/app/StartDirs.h"
#include "gui/common/Localisation.h"

namespace hum {

class StepArrow : public juce::Button {
public:
    explicit StepArrow(bool forward) : juce::Button({}), fwd_(forward) {
        setRepeatSpeed(400, 120);
    }
    std::function<void(bool)> onHover;
    std::function<void(juce::Point<int>)> onRightClick;

    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu() && onRightClick) { onRightClick(e.getScreenPosition()); return; }
        juce::Button::mouseDown(e);
    }

    void mouseEnter(const juce::MouseEvent& e) override {
        juce::Button::mouseEnter(e);
        if (onHover) onHover(true);
    }
    void mouseExit(const juce::MouseEvent& e) override {
        juce::Button::mouseExit(e);
        if (onHover) onHover(false);
    }

    void paintButton(juce::Graphics& g, bool over, bool down) override {
        const auto r = getLocalBounds().toFloat();
        const float w = 7.0f, h = 9.0f;
        const float x = r.getCentreX() - w * 0.5f, y = r.getCentreY() - h * 0.5f;
        juce::Path p;
        if (fwd_) p.addTriangle(x, y, x, y + h, x + w, y + h * 0.5f);
        else      p.addTriangle(x + w, y, x + w, y + h, x, y + h * 0.5f);
        g.setColour(!isEnabled() ? Palette::border.brighter(0.08f)
                    : (down || over) ? Palette::text
                                     : Palette::textDim);
        g.fillPath(p);
    }

private:
    bool fwd_;
};

class PresetField : public juce::Label {
public:
    PresetField() {
        setEditable(false, true, false);
        setFont(juce::FontOptions(12.0f));
        setBorderSize({0, 4, 0, 4});
        setJustificationType(juce::Justification::centred);
        setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::Label::outlineWhenEditingColourId, Palette::accentDim);
    }

    std::function<void()> onBrowse;
    std::function<void(juce::Point<int>)> onActMenu;
    std::function<void(bool)> onHover;

    void setSlot(int n) { if (n != slot_) { slot_ = n; repaint(); } }
    void setDrifted(bool d) { if (d != drift_) { drift_ = d; repaint(); } }
    bool drifted() const { return drift_; }

    void mouseEnter(const juce::MouseEvent& e) override {
        juce::Label::mouseEnter(e);
        hover_ = true;
        if (onHover) onHover(true);
        repaint();
    }
    void mouseExit(const juce::MouseEvent& e) override {
        juce::Label::mouseExit(e);
        hover_ = false;
        if (onHover) onHover(false);
        repaint();
    }

    void mouseDown(const juce::MouseEvent& e) override {
        grabKeyboardFocus();
        if (e.mods.isPopupMenu()) {
            if (onActMenu) onActMenu(e.getScreenPosition());
            return;
        }
        if (onBrowse) onBrowse();
    }

    void paint(juce::Graphics& g) override {
        if (isBeingEdited()) return juce::Label::paint(g);
        auto r = getLocalBounds().reduced(4, 0);
        const auto font = getFont();
        const auto txt = getText();

        const int slotW = slot_ > 0 ? kInk : 0;
        const int textW = juce::GlyphArrangement::getStringWidthInt(font, txt);
        const int groupW = juce::jmin(r.getWidth(), slotW + textW);
        auto grp = r.withWidth(groupW).withX(r.getX() + (r.getWidth() - groupW) / 2);

        if (slot_ > 0) {
            auto gutter = grp.removeFromLeft(kInk);
            g.setColour(drift_ ? Palette::textDim : Palette::accentDim);
            g.setFont(juce::FontOptions(10.5f).withStyle("Bold"));
            g.drawText(juce::String(slot_), gutter.removeFromLeft(kSlot),
                       juce::Justification::centredRight, false);
            if (drift_) {
                const auto c = gutter.toFloat().getCentre();
                g.setColour(Palette::warnAmber());
                g.fillEllipse(c.x - 1.5f, c.y - 1.5f, 3.0f, 3.0f);
            }
        }

        g.setColour(findColour(juce::Label::textColourId));
        g.setFont(font);
        g.drawText(txt, grp, juce::Justification::centred, true);

        if (hover_ && groupW + 22 <= r.getWidth()) {
            juce::Path p;
            const auto c = r.removeFromRight(11).toFloat().getCentre();
            p.addTriangle(c.x - 3.5f, c.y - 1.8f, c.x + 3.5f, c.y - 1.8f, c.x, c.y + 2.2f);
            g.setColour(Palette::textDim);
            g.fillPath(p);
        }
    }

private:
    static constexpr int kSlot = 16;
    static constexpr int kInk  = 28;
    int slot_ = 0;
    bool drift_ = false;
    bool hover_ = false;
};

}
