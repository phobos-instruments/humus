#pragma once
#include <array>
#include <cmath>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "hum/Pattern.h"
#include "gui/ClipDetail.h"
#include "gui/EngineHost.h"
#include "gui/NoteEdit.h"
#include "gui/TimelineChrome.h"
#include "gui/TimelineItems.h"
#include "gui/TracksLayout.h"

namespace hum {

class TracksPane : public juce::Component,
                   public juce::FileDragAndDropTarget,
                   public juce::DragAndDropTarget,
                   public juce::TooltipClient {
public:
    bool isInterestedInDragSource(const SourceDetails&) override;
    void itemDragEnter(const SourceDetails&) override { dropHot_ = true; repaint(); }
    void itemDragExit(const SourceDetails&) override { dropHot_ = false; repaint(); }
    void itemDropped(const SourceDetails&) override;
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray&, int, int) override { dropHot_ = true; repaint(); }
    void fileDragExit(const juce::StringArray&) override { dropHot_ = false; repaint(); }
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    void setPixelsPerBeat(double ppb);
    double pixelsPerBeat() const { return ppb_; }
    double contentEndBeat() const;
    double snapToGrid(double beat) const { return snapBeats(beat, false); }
    void setScrollBeats(double b);
    int rowCount() const { return (int) rows_.size(); }
    std::string selectedNoteNode() const;
    void expandRow(const std::string& node) {
        expanded_.insert(node);
        rebuildSlots();
        repaint();
    }
    std::string addTrack(bool audio, const std::string& target);
    juce::Rectangle<int> clipBounds(int row, const ClipEditor::ClipInfo& ci) const;
    const std::vector<trackslayout::Slot>& slotsForTest() const { return slots_; }
    juce::Rectangle<float> noteBoundsForTest(int clipStart, const NoteEvent& n) const {
        return noteBounds(clipStart, n);
    }
    int selectedNoteCountForTest() const { return (int) selNotes_.size(); }
    int selectedClipCountForTest() const { return (int) selectedClipList().size(); }
    juce::Rectangle<int> rollFieldForTest() const { return rollField(); }
    void selectClipForTest(int row, int clip) { selectClip(row, clip); }
    int rollTopPitchForTest() const { return rollPlot().topPitch; }
    void selectNoteForTest(int clip, int index) { selNotes_ = {{clip, index}}; }
    bool nodeHasMuteParamForTest(const std::string& n) const { return nodeHasMuteParam(n); }
    bool nodeMutedForTest(const std::string& n) const { return nodeMuted(n); }
    juce::Rectangle<int> muteBoxForTest(int row) const { return muteBox(row); }
    void quantiseSelectedNotes(int gridTicks);
    std::string rowNameForTest(int row) const {
        return row >= 0 && row < (int) rows_.size() ? rows_[(size_t) row] : std::string();
    }

    bool timeSelection(double& from, double& to) const;
    void clearTimeSelection() { hasSel_ = false; repaint(); }
    bool followPlayback() const { return follow_; }
    double scrollBeats() const { return scrollBeats_; }

    explicit TracksPane(EngineHost& host) : host_(host) {
        setBufferedToImage(true);
        setOpaque(true);
        setWantsKeyboardFocus(true);
    }

    enum class Mode { Song, Track, Clip, Box };
    Mode mode() const { return mode_; }
    void enterTrackMode(const std::string& node);
    void leaveTrackMode();
    const std::string& trackNode() const { return trackNode_; }
    void enterClipMode(const std::string& node, int clipId);
    void leaveClipMode();
    void enterBoxMode(const std::string& node);
    void leaveBoxMode();
    bool inBoxMode() const { return mode_ == Mode::Box; }
    void commitLine(const std::string& node, const std::string& param,
                    double b0, double v0, double b1, double v1);
    void pencilStep(const std::string& node, const std::string& param, double beat, double v);
    int slotCountForTest() const { return (int) slots_.size(); }
    juce::Rectangle<int> chipBoundsForTest(int row, const ClipEditor::ClipInfo& ci) const {
        return clipChipBounds(row, ci);
    }
    int clickToolForTest(int i) { mouseDownToolbar(toolBox(i).getCentre()); return (int) tool_; }
    void selectPointsForTest(int slot, std::set<int> idx) { selPtSlot_ = slot; selPts_ = std::move(idx); }
    bool copyPastePointsForTest(double atBeat) { copySelectedPoints(); return pastePoints(atBeat); }
    int slotHeightForTest(int i) const { return slots_[(size_t) i].h; }
    juce::Point<int> zoomBoxForTest(int i) const { return zoomBox(i).getCentre(); }
    void setRollRowHForTest(float h) { rollRowH_ = h; repaint(); }
    int slotKindForTest(int i) const { return (int) slots_[(size_t) i].kind; }
    int clipModeId() const { return clipId_; }
    bool clipSelectionTicks(int& from, int& to) const;
    void selectTicksForTest(int from, int to) {
        hasSel_ = true;
        selFrom_ = from / (double) Pattern::kTicksPerBeat;
        selTo_ = to / (double) Pattern::kTicksPerBeat;
    }
    void splitClipSelection();
    bool deleteClipSelection(bool ripple);
    void trimClipToSelection();
    void copyClipSelection();
    bool pasteClipSelection(int atTick);
    void zoomToClip();

    void rebuild();
    void setPlaybackBeat(double beat);
    void setLiveRecording(bool on) { liveRec_ = on; }
    void expandAll() { for (auto& n : rows_) expanded_.insert(n); rebuildSlots(); repaint(); }

    std::function<void(const std::string&)> onSelect;
    std::function<void()> onPatchChanged;
    std::function<void(const std::string&, int)> onOpenClip;
    std::function<void(const std::string&, const std::string&)> onOpenAutomation;
    std::function<void(const std::string&)> onOpenBoxDetail;

    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void traceSel(const char* what, const juce::MouseEvent&) const;
    bool mouseDownToolbar(juce::Point<int> p);
    void mouseDownRuler(const juce::MouseEvent&, juce::Point<int> p);
    bool mouseDownAutoLane(const juce::MouseEvent&, juce::Point<int> p);
    void mouseDownHeader(const juce::MouseEvent&, int row, juce::Point<int> p);
    void mouseDownBody(const juce::MouseEvent&, int row, juce::Point<int> p);
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    static constexpr int kTopH = 18;
    static constexpr int kStripW = 200, kLoopH = 11, kRulerH = 24, kLaneH = 32;
    static constexpr int kLoopGrip = 5;
    static constexpr int kRowHMin = 22, kRowHMax = 180, kRowHDefault = 68;
    int kRowH = kRowHDefault;
    void zoomBy(int axis, double factor);
    static constexpr int kZoomGut = 16, kZoomLen = 140, kZoomCap = 14;
    static constexpr int kDragOutSlack = 24;
    juce::Rectangle<int> zoomSlider(int axis) const {
        return axis == 0 ? juce::Rectangle<int>{getWidth() - kZoomGut - kZoomLen - 4,
                                                getHeight() - kZoomGut + 1, kZoomLen, kZoomGut - 2}
                         : juce::Rectangle<int>{getWidth() - kZoomGut + 1,
                                                getHeight() - kZoomGut - kZoomLen - 4,
                                                kZoomGut - 2, kZoomLen};
    }
    juce::Rectangle<int> zoomBox(int i) const {
        auto s = zoomSlider(i / 2);
        if (i / 2 == 0) return (i % 2) ? s.removeFromRight(kZoomCap) : s.removeFromLeft(kZoomCap);
        return (i % 2) ? s.removeFromTop(kZoomCap) : s.removeFromBottom(kZoomCap);
    }
    juce::Rectangle<int> zoomGroove(int axis) const {
        auto s = zoomSlider(axis);
        return axis == 0 ? s.reduced(kZoomCap, 0) : s.reduced(0, kZoomCap);
    }
    int fieldBottom() const { return getHeight() - kZoomGut; }
    void paintZoom(juce::Graphics& g);
    void paintField(juce::Graphics& g);
    double zoomNorm(int axis) const;
    void setZoomNorm(int axis, double t);
    int zoomDrag_ = -1;
    bool mouseDownZoom(juce::Point<int> p);

    int headerH() const { return kTopH + kLoopH + kRulerH; }
    static constexpr int loopTop() { return kTopH; }
    static constexpr int rulerTop() { return kTopH + kLoopH; }
    float beatToX(double beat) const { return (float) (kStripW + (beat - scrollBeats_) * ppb_); }
    double xToBeat(float x) const { return scrollBeats_ + (x - kStripW) / ppb_; }
    float tickToX(int tick) const { return beatToX(tick / (double) Pattern::kTicksPerBeat); }
    int xToTick(float x) const { return (int) std::llround(xToBeat(x) * Pattern::kTicksPerBeat); }
    int rowTop(int row) const;
    int rowAt(int y) const;
    double gridBeats() const;
    double snapChoice_ = 0.0;
    void cycleSnap();
    void showSnapMenu(juce::Point<int> screenPos);
    double snapBeats(double beat, bool bypass) const;
    int barTicks() const { return host_.automation().timeSigNumerator() * Pattern::kTicksPerBeat; }

    void rebuildSlots();
    void traceView(const char* what) const;
    std::vector<trackslayout::AutoLaneInfo> lanesOf(int row) const;

    bool hasLanes(int row) const { return !lanesOf(row).empty(); }
    bool wantsBoxRow(int row) const;
    int boxRowSlot(int row) const;
    static constexpr int kBoxRowH = 26;
    juce::Rectangle<int> boxFoldBox(const trackslayout::Slot& s) const {
        return {2, s.y + (s.h - 14) / 2, 14, 14};
    }
    void paintBoxRow(juce::Graphics&, const trackslayout::Slot&);
    void paintBoxCrumb(juce::Graphics&);
    void paintBackCrumb(juce::Graphics&);
    void paintCutGuide(juce::Graphics&);
    void paintLinePreview(juce::Graphics&);
    std::string boxNode_;
    int boxLaneH_ = kLaneH;
    int lineSlot_ = -1;
    double lineBeat0_ = 0.0, lineVal0_ = 0.0, lineBeat1_ = 0.0, lineVal1_ = 0.0;
    double pencilLast_ = -1.0, lastPencilVal_ = 0.0;
    void replaceSpan(const std::string& node, const std::string& param, double from, double to,
                     std::vector<AutomationBreakpoint> add, bool openFrom = false);
    bool boxWasExpanded_ = false;
    void paintBoxes(juce::Graphics&, int row);
    bool mouseDownBoxRow(const juce::MouseEvent&, juce::Point<int> p);
    void showBoxMenu(int bx, juce::Point<int> screen);
    void beginBoxDrag(int row, int bx, bool leftEdge, bool rightEdge, juce::Point<int> p);

    std::pair<double, double> laneRange(const std::string& node, const std::string& param) const;
    float laneYAtValue(const trackslayout::Slot& s, double v, double lo, double hi) const;
    double laneValueAtY(const trackslayout::Slot& s, int py, double lo, double hi) const;
    int autoPointAt(const trackslayout::Slot& s, const std::string& node,
                    const std::string& param, juce::Point<int> p) const;

    int clipAt(int row, juce::Point<int> p, bool& leftEdge, bool& rightEdge) const;
    bool nodeHasMuteParam(const std::string& node) const;
    bool nodeMuted(const std::string& node) const;
    void setNodeMuted(const std::string& node, bool muted);

    juce::Rectangle<int> muteBox(int row) const { return {kStripW - 78, rowTop(row) + 4, 22, 14}; }
    juce::Rectangle<int> soloBox(int row) const { return {kStripW - 54, rowTop(row) + 4, 22, 14}; }
    juce::Rectangle<int> recBox(int row) const { return {kStripW - 28, rowTop(row) + 4, 22, 14}; }
    juce::Rectangle<int> foldBox(int row) const { return {2, rowTop(row) + 4, 14, 14}; }
    static constexpr int kFadeGrip = 9;
    static constexpr int kPodH = 26;
    std::set<std::string> collapsedPods_;
    std::string podOfRow(int t) const;
    juce::Rectangle<int> podFoldBox(const trackslayout::Slot& s) const {
        return {2, s.y + (s.h - 12) / 2, 12, 12};
    }
    void paintPodHeader(juce::Graphics&, const trackslayout::Slot&);

    int clipIndexOfId(const std::string& node, int id) const;
    void applyRepeatFill(int endTick);
    bool overRepeatGrip(juce::Rectangle<int> clip, juce::Point<int> p) const {
        return p.y > clip.getBottom() - kFadeGrip && p.x > clip.getRight() - kFadeGrip
               && clip.getWidth() > 3 * kFadeGrip;
    }
    int repeatSrcId_ = 0, repeatStart_ = 0, repeatLen_ = 0;
    std::vector<int> repeatIds_;

    int selPtSlot_ = -1;
    std::set<int> selPts_;
    juce::Rectangle<int> ptMarquee_;
    juce::Point<int> marqueeAnchor_;
    std::vector<AutomationBreakpoint> groupBase_;
    std::set<int> groupSel0_;
    double groupBeat0_ = 0.0, groupVal0_ = 0.0;
    const AutomationLane* laneOfSlot(const trackslayout::Slot&) const;
    bool tryBeginPointSelection(const juce::MouseEvent&, int slot, int hitPoint);
    void updatePointMarquee(juce::Point<int>);
    void dragSelectedPoints(const juce::MouseEvent&);
    bool deleteSelectedPoints();
    void clearPointSelection();
    void paintPointSelection(juce::Graphics&);

    bool hasSel_ = false;
    double selFrom_ = 0.0, selTo_ = 0.0, selAnchor_ = 0.0;
    bool follow_ = false;
    bool liveRec_ = false;
    juce::Rectangle<int> followBox() const { return {kStripW - 68, 1, 64, kTopH - 2}; }

    Mode mode_ = Mode::Song;
    std::string trackNode_;
    int rollTopPitch_ = 83;
    int rollScrollSemis_ = 0;
    float rollScrollAcc_ = 0.0f;
    int rollKeyNote_ = -1;
    void soundRollKey(int pitch);
    void repaintRollKeys() {
        repaint(kStripW - kKeyW, headerH(), kKeyW, getHeight() - headerH());
    }
    float rollRowH_ = 14.0f;
    void fitRoll();
    static constexpr int kChipH = 22;
    static constexpr int kKeyW = 74;
    static constexpr int kRibbonH = 5;
    static constexpr int kVelH = 34;
    static constexpr int kRollMinPaneH = 220;
    bool rollShowsVelocity() const { return fieldBottom() >= kRollMinPaneH; }
    juce::Rectangle<int> rollField() const;
    timelinechrome::RollPlot rollPlot() const;
    void rollPitchRange(int& lo, int& hi) const;
    std::vector<std::string> noteRows() const;
    void stepTrack(int dir);
    juce::Rectangle<int> crumbSongBox() const { return {kStripW + 6, 1, 34, kTopH - 2}; }
    juce::Rectangle<int> crumbNameBox() const { return {kStripW + 44, 1, 150, kTopH - 2}; }
    juce::Rectangle<int> crumbBackBox() const { return {kStripW + 6, 1, 104, kTopH - 2}; }
    int hoverPtSlot_ = -1, hoverPt_ = -1;
    struct PointCopy { double beat, value, valueMax, curve; };
    std::vector<PointCopy> pointClipboard_;
    void copySelectedPoints();
    bool pastePoints(double atBeat);
    enum class RollDrag { None, Move, ResizeL, ResizeR, Velocity, Marquee, Draw, Erase };
    RollDrag rollDrag_ = RollDrag::None;
    std::set<std::pair<int, int>> selNotes_;
    std::map<int, std::vector<NoteEvent>> rollBase_;
    std::set<std::pair<int, int>> rollSelBase_;
    juce::Rectangle<int> rollMarquee_;
    juce::Point<int> rollAnchor_;
    int rollAnchorTick_ = 0, rollAnchorPitch_ = 0, rollAnchorVel_ = 0;
    bool nudgeRunUndoOpen_ = false;

    int noteAt(juce::Point<int> p, int& clip, int& index,
               bool& leftEdge, bool& rightEdge) const;
    juce::Rectangle<float> noteBounds(int clipStart, const NoteEvent& n) const;
    int clipOwning(int absTick) const;
    void snapshotNotes();
    void applyNoteDrag(const juce::MouseEvent&);
    struct NoteKey { int clip = 0, tick = 0, pitch = 0; };
    void commitNotes(std::map<int, std::vector<NoteEvent>> perClip,
                     const std::vector<NoteKey>& keep);
    bool deleteSelectedNotes();
    void showNoteMenu(juce::Point<int> screenPos);
    void nudgeNotes(int dTicks, int dSemis, int dVel);
    void selectAllNotes();
    struct NoteCopy { int tick = 0; NoteEvent n; };
    std::vector<NoteCopy> noteClipboard_;
    int noteClipboardSpan_ = 0;
    void copySelectedNotes();
    bool pasteNotes(int atTick);
    bool duplicateSelectedNotes();
    void eraseNoteUnder(juce::Point<int> p);
    void paintRollSelection(juce::Graphics&);
    bool mouseDownRollGrid(const juce::MouseEvent&, juce::Point<int> p);
    void mouseDragRoll(const juce::MouseEvent&);
    void mouseUpRoll();

    void paintRoll(juce::Graphics&);
    void paintRollCrumb(juce::Graphics&);
    bool mouseDownRoll(const juce::MouseEvent&, juce::Point<int> p);

    static constexpr double kClipMaxPpb = 40000.0;
    std::string clipNode_;
    int clipId_ = -1;
    double songPpb_ = 12.0, songScroll_ = 0.0;
    enum class ClipDrag { None, Select, TrimL, TrimR, StretchL, StretchR, Slip, FadeL, FadeR,
                          CurveL, CurveR, Pending };
    ClipDrag clipDrag_ = ClipDrag::None;
    double clipAnchorBeat_ = 0.0;
    long long clipOffset0_ = 0;
    int clipStart0_ = 0, clipLen0_ = 0;
    clipdetail::SampleWindow clipWindow_;
    int clipOrdinal() const;
    bool clipInfo(ClipEditor::ClipInfo& ci) const;
    juce::Rectangle<int> clipField() const;
    juce::Rectangle<int> clipBox() const;
    double samplesPerBeat() const;
    void paintClip(juce::Graphics&);
    void paintClipWave(juce::Graphics&, juce::Rectangle<int> field,
                       const ClipEditor::ClipInfo& ci, juce::Colour accent);
    void paintClipCrumb(juce::Graphics&);
    void paintClipRibbon(juce::Graphics&, const ClipEditor::ClipInfo& ci);
    bool mouseDownClip(const juce::MouseEvent&, juce::Point<int> p);
    void mouseDragClip(const juce::MouseEvent&);
    void mouseUpClip();
    bool keyPressedClip(const juce::KeyPress&);
    void showClipDetailMenu(juce::Point<int> screenPos);
    void fadeClipToPlayhead(bool in);
    void straightenFade(bool in);
    enum class ClipHit { None, Body, EdgeL, EdgeR, FadeL, FadeR, CurveL, CurveR };
    ClipHit clipEditorHit(juce::Point<int>) const;
    juce::MouseCursor clipEditorCursor(juce::Point<int>) const;
    ClipHit rowClipHit(int row, juce::Point<int>) const;
    void setClipGainDb(double db);
    void loopClipSelection();
    void zoomToSelection();
    void splitClipAt(int tick);
    double transientSense_ = 0.5;
    std::vector<long long> clipTransients(const ClipEditor::ClipInfo& ci);
    long long sampleToTick(const ClipEditor::ClipInfo& ci, long long sessionSample) const;

public:
    bool tabToTransient(int dir);
    void splitAtTransients();
    void toggleClipReverse();
    void normalizeClip();
    void stretchClipBy(double factor);
    void setClipPitch(double semitones);
    int transientCountForTest() {
        ClipEditor::ClipInfo ci;
        return clipInfo(ci) ? (int) clipTransients(ci).size() : 0;
    }

private:

    juce::String getTooltip() override;
    juce::Point<int> hover_;

    bool dropHot_ = false;
    int placeAudioFile(const std::string& node, int atTick, const juce::File& f);
    std::string dropTargetNode(int y);

    juce::Rectangle<int> heldBox(int row) const { return {kStripW - 22, rowTop(row) + 21, 14, 13}; }
    juce::Rectangle<int> heldLaneBox(const trackslayout::Slot& s) const {
        return {kStripW - 28, s.y + (s.h - 13) / 2, 14, 13};
    }

    void paintRuler(juce::Graphics&);
    void paintRow(juce::Graphics&, int row);
    void paintRowHeader(juce::Graphics&, int row, int y);
    void paintAutoLane(juce::Graphics&, const trackslayout::Slot& slot);
    void paintWaveform(juce::Graphics&, juce::Rectangle<int> b, int clipLeft,
                       const ClipEditor::ClipInfo& ci, juce::Colour accent);
    void paintNotes(juce::Graphics&, juce::Rectangle<int> b, int clipLeft,
                    const std::string& node, const ClipEditor::ClipInfo& ci,
                    juce::Colour accent);
    void showClipMenu(int row, int clip, juce::Point<int> screenPos, int atTick);
    bool consolidateRange(int row, double& from, double& to) const;
    void consolidateRow(int row);
    void importAudioInto(const std::string& node, int atTick);
    bool handleStripClick(int slot, juce::Point<int> p);

    juce::Rectangle<int> boxBounds(int row, const PerformanceBox& b) const;
    int boxAt(int row, juce::Point<int> p) const;
    int boxAt(int row, juce::Point<int> p, bool& leftEdge, bool& rightEdge) const;

    EngineHost& host_;
    std::vector<std::string> rows_;
    std::set<std::string> arrangeable_;
    std::set<std::string> expanded_;
    std::vector<trackslayout::Slot> slots_;
    std::unique_ptr<juce::FileChooser> chooser_;
    double ppb_ = 26.0;
    double scrollBeats_ = 0.0;
    double playBeat_ = 0.0;
    int vScroll_ = 0;
    void applyVScroll(int v);
    void mouseMagnify(const juce::MouseEvent&, float scaleFactor) override;
    void zoomAbout(float x, double factor);

    using Tool = noteedit::Tool;
    static constexpr int kToolCount = 5;
    static constexpr Tool kToolbar[kToolCount] = {Tool::Pointer, Tool::Draw, Tool::Line,
                                                  Tool::Scissors, Tool::Eraser};
    Tool tool_ = Tool::Pointer;
    Tool effectiveTool() const;
    juce::Rectangle<int> toolBox(int i) const { return {6 + i * 24, rulerTop() + 1, 22, kRulerH - 3}; }
    juce::Rectangle<int> snapBox() const { return {kStripW - 34, rulerTop() + 1, 32, kRulerH - 3}; }
    juce::Rectangle<int> addTrackBox() const {
        return {6 + kToolCount * 24, rulerTop() + 1, 22, kRulerH - 3};
    }
    void showAddTrackMenu(juce::Point<int> screenPos);
    void repaintCutGuide(int fromX, int toX);
    void paintToolbar(juce::Graphics&);
    void paintDropHint(juce::Graphics&);
    bool applyToolAt(int row, juce::Point<int> p, bool alt);

    enum class Drag { None, ClipMove, ClipResizeL, ClipResizeR, ClipCreate,
                      ClipFadeL, ClipFadeR, ClipFadeCurveL, ClipFadeCurveR, ClipRepeat, ClipMarquee, TimeSelect, PointMarquee,
                      PointGroup, Curve, BoxMove, BoxTrimL, BoxTrimR, Pencil, Line,
                      Scrub, LoopMove, LoopL, LoopR, LoopNew, SongEnd };
    Drag drag_ = Drag::None;
    std::optional<EngineHost::PatternSyncBatch> dragSync_;
    int dragRow_ = -1, dragClip_ = -1;
    int dragGrabTicks_ = 0;
    int dragOriginTick_ = 0;
    std::string dragOriginNode_;
    bool dragDuplicated_ = false, extDrag_ = false;
    void dragClipOut(const std::string& node);
    static constexpr float kChipScale = 2.0f;
    juce::Image clipChip(int row, const ClipEditor::ClipInfo& ci) const;
    juce::Rectangle<int> clipChipBounds(int row, const ClipEditor::ClipInfo& ci) const;
    bool leftForGood(juce::Point<int> p) const;
    int dragAutoSlot_ = -1, dragAutoPoint_ = -1;
    std::string dragAutoNode_, dragAutoParam_;
    enum class AutoGrip { Value, RangeLo, RangeHi, RangeBoth, TriggerTime };
    AutoGrip dragAutoGrip_ = AutoGrip::Value;
    int segmentAt(const trackslayout::Slot&, const std::string& node,
                  const std::string& param, juce::Point<int>) const;
    int dragCurveIndex_ = -1;
    double dragCurve0_ = 0.0;
    int dragCurveY0_ = 0;
    AutoGrip nearerRangeEdge(const trackslayout::Slot&, const std::string& node,
                             const std::string& param, int index, juce::Point<int>) const;
    int selClipRow_ = -1, selClip_ = -1;
    std::set<timeline::ItemRef> sel_;
    class ClipItems : public timeline::ItemKind {
    public:
        explicit ClipItems(TracksPane& p) : pane_(p) {}
        timeline::ItemRef::Kind kind() const override { return timeline::ItemRef::Kind::Clip; }
        std::vector<timeline::ItemRef> all(int row) const override;
        juce::Rectangle<int> bounds(const timeline::ItemRef&) const override;
        bool alive(const timeline::ItemRef&) const override;
        void remove(const std::vector<timeline::ItemRef>&) override;
        void duplicateAfter(const std::vector<timeline::ItemRef>&) override;
        int merge(const std::vector<timeline::ItemRef>&) override;
    private:
        TracksPane& pane_;
    };
    class BoxItems : public timeline::ItemKind {
    public:
        explicit BoxItems(TracksPane& p) : pane_(p) {}
        timeline::ItemRef::Kind kind() const override { return timeline::ItemRef::Kind::Box; }
        std::vector<timeline::ItemRef> all(int row) const override;
        juce::Rectangle<int> bounds(const timeline::ItemRef&) const override;
        bool alive(const timeline::ItemRef&) const override;
        void remove(const std::vector<timeline::ItemRef>&) override;
        void duplicateAfter(const std::vector<timeline::ItemRef>&) override;
        int merge(const std::vector<timeline::ItemRef>&) override;
    private:
        TracksPane& pane_;
    };
    ClipItems clipItems_{*this};
    BoxItems boxItems_{*this};
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
    void selectAll();
    juce::Rectangle<int> clipMarquee_;
    void updateClipMarquee(juce::Point<int> p);
    std::vector<std::pair<int, int>> selectedClipList() const;
    struct ClipCopy { int row = 0; int startTick = 0; PatternChannel data; };
    std::vector<ClipCopy> clipboard_;
    int clipboardSpan_ = 0;
    int clipboardRow_ = -1;
    std::set<std::string> autoOnlyRows_;
    void copySelectedClips();
    bool pasteClips(int atTick, int atRow);
    bool duplicateSelectedClips();
    struct ClipMove { int row = 0, curRow = 0, id = 0, startTick = 0; };
    std::vector<ClipMove> moveBase_;
    int moveGrabRow_ = -1, moveGrabId_ = 0;
    int beginClipMove(int row, int clip, bool duplicate);
    bool rowShiftFits(int deltaRows);
    void moveSelection(int deltaTicks, int deltaRows);
    void restoreClipMove();
    void syncTimeSelection();
    bool edgeScroll(juce::Point<int> p);
    int selectionSpanTicks() const;
    bool selectedClip(std::string& node, ClipEditor::ClipInfo& ci) const;
    void selectClip(int row, int clip) {
        selClipRow_ = row; selClip_ = clip; selBox_ = -1;
        syncLiveTarget();
    }
    void clearClipSel() { selClipRow_ = selClip_ = -1; syncLiveTarget(); }
    void syncLiveTarget() {
        std::vector<std::string> t;
        if (mode_ == Mode::Track) {
            if (!trackNode_.empty()) t.push_back(trackNode_);
        } else {
            for (const auto& n : rows_)
                if (!host_.nodeRecordsAudio(n) && host_.midi().isRecordTarget(n))
                    t.push_back(n);
            if (t.empty())
                if (const auto sel = selectedNoteNode(); !sel.empty()) t.push_back(sel);
        }
        host_.setLiveTargets(std::move(t));
    }

    int selBox_ = -1, dragBox_ = -1;
    double boxAnchor_ = 0.0, boxDragDelta_ = 0.0;
    double boxTrimL_ = 0.0, boxTrimR_ = 0.0;
    double boxOrigS_ = 0.0, boxOrigE_ = 0.0;
    double loopAnchor_ = 0.0;
    double loopOrigStart_ = 0.0, loopOrigEnd_ = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TracksPane)
};

}
