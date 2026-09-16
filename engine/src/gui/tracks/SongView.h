// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <cmath>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "hum/Pattern.h"
#include "gui/tracks/ClipDetail.h"
#include "gui/host/EngineHostAutomation.h"
#include "gui/host/EngineHostClips.h"
#include "gui/host/EngineHostMidiControl.h"
#include "gui/host/EngineHostPattern.h"
#include "gui/host/TracksHost.h"
#include "gui/tracks/TimelineContext.h"
#include "gui/tracks/TimelineView.h"
#include "gui/tracks/TracksGeometry.h"
#include "gui/pianoroll/NoteEdit.h"
#include "gui/tracks/TimelineChips.h"
#include "gui/tracks/TimelineItems.h"
#include "gui/tracks/TracksLayout.h"
#include "gui/tracks/ZoomBar.h"

namespace hum {

class SongView : public juce::Component, public juce::TooltipClient {
public:
    class Context : public TimelineContext {
    public:
        virtual std::string pinnedRow() const = 0;
        virtual void enterTrack(const std::string& node) = 0;
        virtual void enterClip(const std::string& node, int clipId) = 0;
        virtual void liveTargetsChanged() = 0;
        virtual void nodeSelected(const std::string& node) = 0;
        virtual void patchChanged() = 0;
        virtual void openClip(const std::string& node, int clip) = 0;
        virtual void openAutomation(const std::string& node, const std::string& param) = 0;
        virtual void openBoxDetail(const std::string& node) = 0;
    };

    SongView(Context& ctx, TimelineView& view) : ctx_(ctx), view_(view) { setOpaque(true); }

    void rebuild();
    void rebuildSlots();
    void applyVScroll(int v);
    int rowHeight() const { return rowH_; }
    void setRowHeight(int h);
    static constexpr int kRowHMin = 22, kRowHMax = 180;

    bool inBox() const { return !boxNode_.empty(); }
    const std::string& boxNode() const { return boxNode_; }
    void enterBox(const std::string& node);
    void leaveBox();
    bool dragging() const { return drag_ != Drag::None; }
    void clearSelections();
    std::vector<std::string> liveTargets() const;
    std::vector<std::string> noteRows() const;
    void repaintCutGuide();
    void repaintRecordingRows();

    bool keyPressed(const juce::KeyPress&) override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    juce::String getTooltip() override;

    bool isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails&);
    void itemDropped(const juce::DragAndDropTarget::SourceDetails&);
    static bool isInterestedInFileDrag(const juce::StringArray& files);
    void filesDropped(const juce::StringArray& files, int x, int y);
    void setDropHot(bool hot);

    int rowCount() const { return (int) rows_.size(); }
    std::string rowNameForTest(int row) const {
        return row >= 0 && row < (int) rows_.size() ? rows_[(size_t) row] : std::string();
    }
    std::string selectedNoteNode() const;
    void expandRow(const std::string& node);
    void expandAll();
    std::string addTrack(bool audio, const std::string& target);
    std::string addMidiTrack();
    void showAddTrackMenu(juce::Point<int> screenPos);
    void placeTracedMelody(const ClipEditor::ClipInfo& ci, const std::vector<NoteEvent>& notes);
    int placeMidiFile(const std::string& node, int atTick, const juce::File& f);
    std::string dropTargetMidiNode(int y);
    void convertClipToMidi(const std::string& node, const ClipEditor::ClipInfo& ci);
    void stretchClip(const std::string& node, int clip, const ClipEditor::ClipInfo& ci, double factor);
    juce::Rectangle<int> clipBounds(int row, const ClipEditor::ClipInfo& ci) const;
    static bool showsFilmstrip(const ClipEditor::ClipInfo& ci) { return ci.isVideo || ci.isCompound; }
    void deleteRow(const std::string& node);
    int cutAtPlayhead();
    int clipsUnderPlayhead() const;
    bool ownsRow(const std::string& node) const;
    void selectTrack(int row, bool range, bool toggle);
    std::vector<std::string> selectedTracks() const;
    void deleteRows(const std::vector<std::string>& nodes);
    bool deleteSelectedTracks();
    void commitLine(const std::string& node, const std::string& param,
                    double b0, double v0, double b1, double v1);
    void pencilStep(const std::string& node, const std::string& param, double beat, double v);
    int clipIndexOfId(const std::string& node, int id) const;
    void setClipboard(PatternChannel data, int spanTicks);
    bool pasteClipboardInto(const std::string& node, int atTick);

    const std::vector<trackslayout::Slot>& slotsForTest() const { return slots_; }
    int slotCountForTest() const { return (int) slots_.size(); }
    int slotHeightForTest(int i) const { return slots_[(size_t) i].h; }
    int slotKindForTest(int i) const { return (int) slots_[(size_t) i].kind; }
    int selectedClipCountForTest() const { return (int) selectedClipList().size(); }
    void selectClipForTest(int row, int clip) { selectClip(row, clip); }
    void selectTrackForTest(int row, bool range, bool toggle) { selectTrack(row, range, toggle); }
    int selectedRowForTest() const { return selClipRow_; }
    int mergeSelectionForTest() { return mergeSelection(); }
    juce::String mergeRefusalForTest() const { return mergeRefusal(); }
    bool nodeHasMuteParamForTest(const std::string& n) const { return nodeHasMuteParam(n); }
    bool nodeMutedForTest(const std::string& n) const { return nodeMuted(n); }
    juce::Rectangle<int> muteBoxForTest(int row) const { return muteBox(row); }
    juce::Rectangle<int> chipBoundsForTest(int row, const ClipEditor::ClipInfo& ci) const {
        return clipChipBounds(row, ci);
    }
    void selectPointsForTest(int slot, std::set<int> idx) { selPtSlot_ = slot; selPts_ = std::move(idx); }
    bool copyPastePointsForTest(double atBeat) { copySelectedPoints(); return pastePoints(atBeat); }

private:
    using Tool = noteedit::Tool;
    static constexpr int kTopH = tracksgeo::kTopH;
    static constexpr int kStripW = tracksgeo::kStripW, kLaneH = 32;
    static constexpr int kClipEdgeGrab = tracksgeo::kClipEdgeGrab;
    static constexpr int kFadeGrip = tracksgeo::kFadeGrip;
    static constexpr int kRowHDefault = 68, kBoxRowH = 26, kPodH = 26;
    static constexpr int kDragOutSlack = 24;
    static constexpr int kMaxReelPaintDepth = 4;
    static constexpr float kChipScale = 2.0f;

    TracksHost& host() const { return ctx_.timelineHost(); }
    juce::Point<int> paneToScreen(juce::Point<int> p) const { return localPointToGlobal(p - getPosition()); }
    juce::MouseEvent paneEvent(const juce::MouseEvent& e) const { return e.getEventRelativeTo(getParentComponent()); }
    void repaintPane(juce::Rectangle<int> r) { repaint(r - getPosition()); }
    void repaintAll() { ctx_.viewChanged(); }
    void repaintRow(int row);
    void traceRows(const char* what) const;
    void traceSel(const char* what, const juce::MouseEvent&) const;

    static constexpr int headerH() { return tracksgeo::headerH(); }
    static constexpr int rulerTop() { return tracksgeo::rulerTop(); }
    float beatToX(double beat) const { return view_.beatToX(beat, kStripW); }
    double xToBeat(float x) const { return view_.xToBeat(x, kStripW); }
    float tickToX(int tick) const { return view_.tickToX(tick, kStripW); }
    int xToTick(float x) const { return view_.xToTick(x, kStripW); }
    double gridBeats() const { return ctx_.gridBeats(); }
    double snapBeats(double beat, bool bypass) const { return ctx_.snapBeats(beat, bypass); }
    Tool effectiveTool() const { return ctx_.effectiveTool(); }
    int barTicks() const {
        return std::max(1, (int) std::lround(host().automation().meterAt(host().positionBeats()).quarterNotesPerBar()
                                             * Pattern::kTicksPerBeat));
    }
    int rowTop(int row) const;
    int rowAt(int y) const;

    std::vector<trackslayout::AutoLaneInfo> lanesOf(int row) const;
    bool hasLanes(int row) const { return !lanesOf(row).empty(); }
    bool wantsBoxRow(int row) const;
    int boxRowSlot(int row) const;
    std::string podOfRow(int t) const;
    juce::Rectangle<int> boxFoldBox(const trackslayout::Slot& s) const {
        return {2, s.y + (s.h - 14) / 2, 14, 14};
    }
    juce::Rectangle<int> podFoldBox(const trackslayout::Slot& s) const {
        return {2, s.y + (s.h - 12) / 2, 12, 12};
    }
    juce::Rectangle<int> muteBox(int row) const { return {kStripW - 78, rowTop(row) + 4, 22, 14}; }
    juce::Rectangle<int> soloBox(int row) const { return {kStripW - 54, rowTop(row) + 4, 22, 14}; }
    juce::Rectangle<int> recBox(int row) const { return {kStripW - 28, rowTop(row) + 4, 22, 14}; }
    juce::Rectangle<int> foldBox(int row) const { return {2, rowTop(row) + 4, 14, 14}; }
    juce::Rectangle<int> destBox(int row, const std::string& node) const {
        return timelinechrome::midiDestChipRect(
                   {30, rowTop(row) + 18, kStripW - 68, 17}, host().model(), node)
            .expanded(2);
    }
    juce::Rectangle<int> heldBox(int row) const { return {kStripW - 22, rowTop(row) + 21, 14, 13}; }
    juce::Rectangle<int> heldLaneBox(const trackslayout::Slot& s) const {
        return {kStripW - 28, s.y + (s.h - 13) / 2, 14, 13};
    }
    bool overRepeatGrip(juce::Rectangle<int> clip, juce::Point<int> p) const {
        return p.y > clip.getBottom() - kFadeGrip && p.x > clip.getRight() - kFadeGrip
               && clip.getWidth() > 3 * kFadeGrip;
    }

    void paintField(juce::Graphics&);
    void paintTimeSelection(juce::Graphics&);
    void paintDropHint(juce::Graphics&);
    void paintRow(juce::Graphics&, int row);
    void paintRowHeader(juce::Graphics&, int row, int y);
    void paintAutoLane(juce::Graphics&, const trackslayout::Slot& slot);
    void paintPodHeader(juce::Graphics&, const trackslayout::Slot&);
    void paintBoxRow(juce::Graphics&, const trackslayout::Slot&);
    void paintBoxes(juce::Graphics&, int row);
    void paintLinePreview(juce::Graphics&);
    void paintPointSelection(juce::Graphics&);
    bool paintTapeTiles(juce::Graphics&, juce::Rectangle<int>, const ClipEditor::ClipInfo&);
    bool paintReelTiles(juce::Graphics&, juce::Rectangle<int>, const ClipEditor::ClipInfo&,
                        int depth = 0);
    void paintFilmstrip(juce::Graphics&, juce::Rectangle<int> b,
                        const ClipEditor::ClipInfo& ci, juce::Colour ink);
    void paintWaveform(juce::Graphics&, juce::Rectangle<int> b, int clipLeft,
                       const ClipEditor::ClipInfo& ci, juce::Colour accent);
    void paintNotes(juce::Graphics&, juce::Rectangle<int> b, int clipLeft,
                    const std::string& node, const ClipEditor::ClipInfo& ci,
                    juce::Colour accent);
    double clipSecondsAtX(const ClipEditor::ClipInfo& ci, int x) const;

    void mouseDownAt(const juce::MouseEvent&);
    bool mouseDownAutoLane(const juce::MouseEvent&, juce::Point<int> p);
    void mouseDownHeader(const juce::MouseEvent&, int row, juce::Point<int> p);
    void mouseDownBody(const juce::MouseEvent&, int row, juce::Point<int> p);
    bool mouseDownBoxRow(const juce::MouseEvent&, juce::Point<int> p);
    void mouseDragAt(const juce::MouseEvent&);
    void mouseUpAt(const juce::MouseEvent&);
    void mouseMoveAt(const juce::MouseEvent&);
    void mouseDoubleClickAt(const juce::MouseEvent&);
    bool applyToolAt(int row, juce::Point<int> p, bool alt);
    bool selectedClip(std::string& node, ClipEditor::ClipInfo& ci) const;

    void showBoxMenu(int bx, juce::Point<int> screen);
    void beginBoxDrag(int row, int bx, bool leftEdge, bool rightEdge, juce::Point<int> p);
    void replaceSpan(const std::string& node, const std::string& param, double from, double to,
                     std::vector<AutomationBreakpoint> add, bool openFrom = false);
    std::pair<double, double> laneRange(const std::string& node, const std::string& param) const;
    float laneYAtValue(const trackslayout::Slot& s, double v, double lo, double hi) const;
    double laneValueAtY(const trackslayout::Slot& s, int py, double lo, double hi) const;
    void editAutoPoint(const std::string& node, const std::string& param, int index,
                       juce::Point<int> screenAt);
    int pointIndexAtBeat(const std::string& node, const std::string& param, double beat) const;
    void addPointAtClick(const juce::MouseEvent& e, int slot);
    int autoPointAt(const trackslayout::Slot& s, const std::string& node,
                    const std::string& param, juce::Point<int> p) const;
    enum class AutoGrip { Value, RangeLo, RangeHi, RangeBoth, TriggerTime };
    int segmentAt(const trackslayout::Slot&, const std::string& node,
                  const std::string& param, juce::Point<int>) const;
    AutoGrip nearerRangeEdge(const trackslayout::Slot&, const std::string& node,
                             const std::string& param, int index, juce::Point<int>) const;

    int clipAt(int row, juce::Point<int> p, bool& leftEdge, bool& rightEdge) const;
    enum class ClipHit { None, Body, EdgeL, EdgeR, FadeL, FadeR, CurveL, CurveR };
    ClipHit rowClipHit(int row, juce::Point<int>) const;
    juce::Rectangle<int> boxBounds(int row, const PerformanceBox& b) const;
    int boxAt(int row, juce::Point<int> p) const;
    int boxAt(int row, juce::Point<int> p, bool& leftEdge, bool& rightEdge) const;
    bool nodeHasMuteParam(const std::string& node) const;
    bool nodeMuted(const std::string& node) const;
    void setNodeMuted(const std::string& node, bool muted);
    double takeStartBeat(const std::string& node) const;
    bool rowRecording(int row, const std::vector<std::string>& live) const;

    void applyRepeatFill(int endTick);
    int placeAudioFile(const std::string& node, int atTick, const juce::File& f);
    int placeVideoFile(const std::string& node, int atTick, const juce::File& f);
    std::string dropTargetNode(int y, bool video);
    void dropPad(const std::string& pad, int index, juce::Point<int> at);
    void showClipMenu(int row, int clip, juce::Point<int> screenPos, int atTick);
    bool consolidateRange(int row, double& from, double& to) const;
    void consolidateRow(int row);
    void importAudioInto(const std::string& node, int atTick);
    void dragClipOut(const std::string& node);
    juce::Image clipChip(int row, const ClipEditor::ClipInfo& ci) const;
    juce::Rectangle<int> clipChipBounds(int row, const ClipEditor::ClipInfo& ci) const;
    bool leftForGood(juce::Point<int> p) const;

    const AutomationLane* laneOfSlot(const trackslayout::Slot&) const;
    bool tryBeginPointSelection(const juce::MouseEvent&, int slot, int hitPoint);
    void updatePointMarquee(juce::Point<int>);
    void dragSelectedPoints(const juce::MouseEvent&);
    bool deleteSelectedPoints();
    void clearPointSelection();
    void copySelectedPoints();
    bool pastePoints(double atBeat);

    class ClipItems : public timeline::ItemKind {
    public:
        explicit ClipItems(SongView& s) : song_(s) {}
        timeline::ItemRef::Kind kind() const override { return timeline::ItemRef::Kind::Clip; }
        std::vector<timeline::ItemRef> all(int row) const override;
        juce::Rectangle<int> bounds(const timeline::ItemRef&) const override;
        bool alive(const timeline::ItemRef&) const override;
        void remove(const std::vector<timeline::ItemRef>&) override;
        void duplicateAfter(const std::vector<timeline::ItemRef>&) override;
        int merge(const std::vector<timeline::ItemRef>&) override;
    private:
        SongView& song_;
    };
    class BoxItems : public timeline::ItemKind {
    public:
        explicit BoxItems(SongView& s) : song_(s) {}
        timeline::ItemRef::Kind kind() const override { return timeline::ItemRef::Kind::Box; }
        std::vector<timeline::ItemRef> all(int row) const override;
        juce::Rectangle<int> bounds(const timeline::ItemRef&) const override;
        bool alive(const timeline::ItemRef&) const override;
        void remove(const std::vector<timeline::ItemRef>&) override;
        void duplicateAfter(const std::vector<timeline::ItemRef>&) override;
        int merge(const std::vector<timeline::ItemRef>&) override;
    private:
        SongView& song_;
    };
    std::array<timeline::ItemKind*, 2> kinds() { return {&clipItems_, &boxItems_}; }
    timeline::ItemRef clipRef(int row, int clipOrdinal) const;
    timeline::ItemRef boxRef(int row, int box) const { return {timeline::ItemRef::Kind::Box, row, box}; }
    bool selected(const timeline::ItemRef& r) const { return sel_.count(r) != 0; }
    bool clipSelected(int row, int id) const { return selected({timeline::ItemRef::Kind::Clip, row, id}); }
    bool boxSelected(int box) const;
    int selectedClipsN() const;
    int selectedBoxesN() const;
    void toggleSelected(const timeline::ItemRef& r);
    void clearSelection() { sel_.clear(); }
    std::vector<timeline::ItemRef> selectedOf(timeline::ItemRef::Kind k) const;
    void marqueeSelect(juce::Rectangle<int> area);
    bool deleteSelection();
    bool duplicateSelection();
    int mergeSelection();
    juce::String mergeRefusal() const;
    void selectAll();
    void updateClipMarquee(juce::Point<int> p);
    std::vector<std::pair<int, int>> selectedClipList() const;
    void copySelectedClips();
    bool pasteClips(int atTick, int atRow);
    bool duplicateSelectedClips();
    int beginClipMove(int row, int clip, bool duplicate);
    bool rowShiftFits(int deltaRows);
    void moveSelection(int deltaTicks, int deltaRows);
    void restoreClipMove();
    void syncTimeSelection();
    int selectionSpanTicks() const;
    void selectClip(int row, int clip) {
        selClipRow_ = row; selClip_ = clip; selBox_ = -1;
        ctx_.liveTargetsChanged();
    }
    void clearClipSel() { selClipRow_ = selClip_ = -1; ctx_.liveTargetsChanged(); }

    Context& ctx_;
    TimelineView& view_;
    std::vector<std::string> rows_;
    std::set<std::string> arrangeable_;
    std::set<std::string> expanded_;
    std::set<std::string> autoOnlyRows_;
    std::set<std::string> collapsedPods_;
    std::vector<trackslayout::Slot> slots_;
    int rowH_ = kRowHDefault;
    std::unique_ptr<juce::FileChooser> chooser_;
    juce::Point<int> hover_;
    bool dropHot_ = false;

    std::string boxNode_;
    bool boxWasExpanded_ = false;
    double boxPpb_ = 12.0, boxScroll_ = 0.0;
    int boxLaneH_ = kLaneH;
    int lineSlot_ = -1;
    double lineBeat0_ = 0.0, lineVal0_ = 0.0, lineBeat1_ = 0.0, lineVal1_ = 0.0;
    double pencilLast_ = -1.0, lastPencilVal_ = 0.0;

    int repeatSrcId_ = 0, repeatStart_ = 0, repeatLen_ = 0;
    std::vector<int> repeatIds_;

    int selPtSlot_ = -1;
    std::set<int> selPts_;
    juce::Rectangle<int> ptMarquee_;
    juce::Point<int> marqueeAnchor_;
    std::vector<AutomationBreakpoint> groupBase_;
    std::set<int> groupSel0_;
    double groupBeat0_ = 0.0, groupVal0_ = 0.0;
    int hoverPtSlot_ = -1, hoverPt_ = -1;
    struct PointCopy { double beat, value, valueMax, curve; };
    std::vector<PointCopy> pointClipboard_;
    std::set<std::string> selTracks_;

    enum class Drag { None, ClipMove, ClipResizeL, ClipResizeR, ClipCreate,
                      ClipFadeL, ClipFadeR, ClipFadeCurveL, ClipFadeCurveR, ClipRepeat, ClipMarquee, PointMarquee,
                      PointGroup, Curve, BoxMove, BoxTrimL, BoxTrimR, Pencil, Line };
    Drag drag_ = Drag::None;
    std::optional<PatternSyncHold> dragSync_;
    int dragRow_ = -1, dragClip_ = -1;
    int dragGrabTicks_ = 0;
    int dragOriginTick_ = 0;
    std::string dragOriginNode_;
    bool dragDuplicated_ = false, extDrag_ = false;
    int dragAutoSlot_ = -1, dragAutoPoint_ = -1;
    std::string dragAutoNode_, dragAutoParam_;
    AutoGrip dragAutoGrip_ = AutoGrip::Value;
    int dragCurveIndex_ = -1;
    double dragCurve0_ = 0.0;
    int dragCurveY0_ = 0;

    int selClipRow_ = -1, selClip_ = -1;
    std::set<timeline::ItemRef> sel_;
    ClipItems clipItems_{*this};
    BoxItems boxItems_{*this};
    juce::Rectangle<int> clipMarquee_;
    struct ClipCopy { int row = 0; int startTick = 0; PatternChannel data; };
    std::vector<ClipCopy> clipboard_;
    int clipboardSpan_ = 0;
    int clipboardRow_ = -1;
    int clipboardTick_ = -1;
    int pasteTickFor(int atTick, int atRow) const {
        const bool ontoSource = atRow == clipboardRow_ && atTick == clipboardTick_;
        return ontoSource ? atTick + clipboardSpan_ : atTick;
    }
    struct ClipMove { int row = 0, curRow = 0, id = 0, startTick = 0; };
    std::vector<ClipMove> moveBase_;
    int moveGrabRow_ = -1, moveGrabId_ = 0;

    int selBox_ = -1, dragBox_ = -1;
    double boxAnchor_ = 0.0, boxDragDelta_ = 0.0;
    double boxTrimL_ = 0.0, boxTrimR_ = 0.0;
    double boxOrigS_ = 0.0, boxOrigE_ = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SongView)
};

}
