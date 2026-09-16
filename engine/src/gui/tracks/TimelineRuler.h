// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <optional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/host/BrickHost.h"
#include "gui/tracks/TimelineContext.h"
#include "gui/tracks/TimelineView.h"

namespace hum {

class TimelineRuler : public juce::Component, public juce::TooltipClient {
public:
    class Context : public TimelineContext {
    public:
        virtual bool showsSongEnd() const = 0;
        virtual void paintCrumb(juce::Graphics& g) = 0;
        virtual bool crumbDown(const juce::MouseEvent& e, juce::Point<int> p) = 0;
        virtual juce::String crumbTooltip(juce::Point<int> p) = 0;
        virtual void showAddTrackMenu(juce::Point<int> screen) = 0;
    };

    TimelineRuler(Context& ctx, TimelineView& view) : ctx_(ctx), view_(view) { setOpaque(true); }

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    juce::String getTooltip() override;

    bool pressToolbar(juce::Point<int> p);
    static noteedit::Tool toolAt(int slot);
    void showSnapMenu(juce::Point<int> screen);

private:
    enum class Drag { None, Scrub, SongEnd, TimeSelect, LoopNew, LoopL, LoopR, LoopMove };

    float beatToX(double beat) const;
    double xToBeat(float x) const;
    bool overLoopLane(juce::Point<int> p) const;
    void paintBars(juce::Graphics& g);
    void paintTools(juce::Graphics& g);
    void pressRuler(const juce::MouseEvent& e, juce::Point<int> p);
    void showLoopMenu(juce::Point<int> at);
    void repaintLoopLane();
    void selectionChanged(double oldFrom, double oldTo, bool wasActive);

    Context& ctx_;
    TimelineView& view_;
    Drag drag_ = Drag::None;
    std::optional<PatternSyncHold> sync_;
    double loopAnchor_ = 0.0, loopOrigStart_ = 0.0, loopOrigEnd_ = 0.0;
    bool loopDrawn_ = false;
    juce::Point<int> hover_;
};

}
