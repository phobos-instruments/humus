// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/host/EngineHostClips.h"
#include "gui/tracks/NotePlot.h"
#include "gui/tracks/TimelineContext.h"
#include "gui/tracks/TimelineView.h"
#include "gui/tracks/TracksGeometry.h"

namespace hum {

class TrackRollView : public juce::Component, public juce::TooltipClient {
public:
    class Context : public TimelineContext {
    public:
        virtual void rebuildRows() = 0;
        virtual void leaveTrack() = 0;
        virtual void stepTrackBy(int dir) = 0;
    };

    TrackRollView(Context& ctx, TimelineView& view) : ctx_(ctx), view_(view) { setOpaque(true); }

    void open(const std::string& node);
    void close();
    const std::string& node() const { return node_; }
    void fitRoll();
    float rowHeight() const { return rowH_; }
    void setRowHeight(float h);

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    juce::String getTooltip() override;
    bool keyPressed(const juce::KeyPress& k) override;
    void repaintCutGuide();

    bool hasNoteSelection() const { return !sel_.empty(); }
    void clearNoteSelection();
    int selectedNoteCount() const { return (int) sel_.size(); }
    void selectNote(int clip, int index) { sel_ = {{clip, index}}; }
    void nudgeNotes(int dTicks, int dSemis, int dVel);
    void selectAllNotes();
    void copySelectedNotes();
    bool pasteNotes(int atTick);
    bool deleteSelectedNotes();
    bool duplicateSelectedNotes();
    void quantiseSelectedNotes(int gridTicks);
    int selectedClip() const { return selClip_; }
    void selectClip(int clip);
    void resizeSelectedClipTo(int absTick, bool fromLeft);
    juce::Rectangle<float> noteBounds(int clipStart, const NoteEvent& n) const;
    juce::Rectangle<int> rollField() const;
    timelinechrome::RollPlot rollPlot() const;

private:
    enum class Drag { None, Move, ResizeL, ResizeR, Velocity, VelLane, VelDivider, Marquee, Draw, Erase,
                      ClipL, ClipR };
    struct NoteKey { int clip = 0, tick = 0, pitch = 0; };
    struct NoteCopy { int tick = 0; NoteEvent n; };
    static constexpr int kKeyW = 74;
    static constexpr int kRibbonH = 5;
    static constexpr int kRollMinPaneH = 220;

    TracksHost& host() const { return ctx_.timelineHost(); }
    juce::Point<int> toPane(juce::Point<int> p) const { return p + getPosition(); }
    float tickToX(int tick) const { return view_.tickToX(tick, tracksgeo::kStripW); }
    double xToBeat(float x) const { return view_.xToBeat(x, tracksgeo::kStripW); }
    int xToTick(float x) const { return view_.xToTick(x, tracksgeo::kStripW); }
    bool rollShowsVelocity() const { return getBottom() >= kRollMinPaneH; }
    int velAtY(int y) const;
    void repaintRollKeys();
    void rollPitchRange(int& lo, int& hi) const;
    int noteAt(juce::Point<int> p, int& clip, int& index, bool& leftEdge, bool& rightEdge) const;
    int clipOwning(int absTick) const;
    int ribbonTop() const { return rollField().getY() - kRibbonH; }
    juce::Rectangle<int> clipHandle(bool left) const;
    bool clipHandleAt(juce::Point<int> p, bool& left) const;
    void paintClipHandles(juce::Graphics& g);
    void snapshotNotes();
    void applyNoteDrag(const juce::MouseEvent& e);
    void applyVelLaneEdit(juce::Point<int> p);
    void applyVelLaneLine(juce::Point<int> a, juce::Point<int> b);
    void commitNotes(std::map<int, std::vector<NoteEvent>> perClip, const std::vector<NoteKey>& keep);
    void eraseNoteUnder(juce::Point<int> p);
    void showNoteMenu(juce::Point<int> screenPos);
    void soundRollKey(int pitch);
    bool mouseDownRoll(const juce::MouseEvent& e, juce::Point<int> p);
    bool mouseDownRollGrid(const juce::MouseEvent& e, juce::Point<int> p);
    void mouseDragRoll(const juce::MouseEvent& e);
    void mouseUpRoll();
    void paintRoll(juce::Graphics& g);
    void paintRollSelection(juce::Graphics& g);

    Context& ctx_;
    TimelineView& view_;
    std::string node_;
    int topPitch_ = 83;
    int scrollSemis_ = 0;
    float scrollAcc_ = 0.0f;
    int keyNote_ = -1;
    float rowH_ = 14.0f;
    int velH_ = 56;
    Drag drag_ = Drag::None;
    std::set<std::pair<int, int>> sel_;
    std::map<int, std::vector<NoteEvent>> base_;
    std::set<std::pair<int, int>> selBase_;
    juce::Rectangle<int> marquee_;
    juce::Point<int> anchor_;
    int anchorTick_ = 0, anchorPitch_ = 0, anchorVel_ = 0;
    bool nudgeRunUndoOpen_ = false;
    juce::Point<int> velAnchor_;
    bool velLine_ = false;
    int velShowX_ = -1, velShowVal_ = -1;
    std::vector<NoteCopy> clipboard_;
    int clipboardSpan_ = 0;
    juce::Point<int> hover_{-1, -1};
    int selClip_ = -1;
};

}
