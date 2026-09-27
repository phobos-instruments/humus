// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/Faceplate.h"
#include "gui/editor/FaceplateOwner.h"
#include "gui/editor/LayoutCondition.h"
#include "gui/editor/OrganismEditor.h"
#include "gui/editor/Skin.h"
#include "gui/editor/juce/JuceBrickViews.h"
#include "gui/editor/juce/PanelScroller.h"
#include "gui/host/EditorHost.h"
#include "hum/LayoutSpec.h"

namespace hum {

class LayoutEditor : public OrganismEditor, private FaceplateOwner {
public:
    LayoutEditor(EditorHost& host, std::string organism, LayoutSpec spec)
        : host_(host), name_(std::move(organism)), spec_(std::move(spec)), skin_(Skin::forBlueprint(spec_)),
          views_(*this, host), face_(host, name_, spec_, views_, *this) {
        setOpaque(true);
        face_.build();
        adoptScrolledViews();
    }

    void reloadValues() override {
        face_.reloadValues();
        fitScrollExtent();
    }
    void reloadTextValues() override {
        face_.reloadTextValues();
        fitScrollExtent();
    }
    void refreshAutomatedValues() override {
        face_.refreshLive();
        fitScrollExtent();
    }
    void openClip(int clip) override { face_.openClip(clip); }

    int preferredContentWidth() const override { return spec_.width; }
    int preferredContentHeight(int width) const override {
        const int c = collarHeight();
        if (spec_.resize == LayoutSpec::Resize::Stretch) return spec_.height + c;
        return (int) std::lround(spec_.height * scale(width)) + c;
    }

    void paint(juce::Graphics& g) override;
    void resized() override;
    void setLayoutDeferred(bool deferred) override {
        deferLayout_ = deferred;
        if (!deferred && layoutDirty_) {
            layoutDirty_ = false;
            resized();
        }
    }

    const Skin* skinForTest() const { return skin_.empty() ? nullptr : &skin_; }
    int collarHeight() const;
    static constexpr double kMaxGrow = 2.5;

    juce::ComboBox* comboFor(const std::string& param) const;
    bool scrolls() const { return spec_.scrollFrom >= 0; }
    PanelScroller* scrollerForTest() { return scrolls() ? &scroller_ : nullptr; }
    int scrollContentHeightForTest() const { return scroller_.contentHeight(); }
    int controlsLeftBelowScrollLineForTest() const {
        int left = 0;
        for (auto* child : getChildren())
            if (child != &scroller_ && child->getY() >= scrollTop()) ++left;
        return left;
    }
    Faceplate& face() { return face_; }
    const Faceplate& face() const { return face_; }

private:
    void automationChanged() override { if (onAutomationChanged) onAutomationChanged(); }
    void openMenu(const std::string& param, Point at) override;
    int coverTicks() const override;
    unsigned nowMs() const override { return (unsigned) juce::Time::getMillisecondCounter(); }
    void repaintFace() override { repaint(); }
    void gatesApplied() override { fitScrollExtent(); }
    bool knowsClass(const std::string& className) const override;
    void paintSkin(juce::Graphics& g);
    void adoptScrolledViews();
    int scrollTop() const;
    void placeScrolledViews();
    void fitScrollExtent();

    double scale(int width) const {
        if (spec_.width <= 0) return 1.0;
        const double want = (double) width / (double) spec_.width;
        if (spec_.resize == LayoutSpec::Resize::Grow) return std::min(want, kMaxGrow);
        return std::min(1.0, want);
    }

    EditorHost& host_;
    std::string name_;
    LayoutSpec spec_;
    Skin skin_;
    PanelScroller scroller_;
    juce::Component scrollContent_;
    std::vector<juce::Component*> scrolled_;
    JuceBrickViews views_;
    Faceplate face_;
    bool deferLayout_ = false, layoutDirty_ = false;
};

}
