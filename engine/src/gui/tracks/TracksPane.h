// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "hum/Pattern.h"
#include "gui/host/TracksHost.h"
#include "gui/pianoroll/NoteEdit.h"
#include "gui/tracks/ClipEditorView.h"
#include "gui/tracks/SongView.h"
#include "gui/tracks/TimelineRuler.h"
#include "gui/tracks/TimelineView.h"
#include "gui/tracks/TrackRollView.h"
#include "gui/tracks/TracksGeometry.h"
#include "gui/tracks/ZoomBar.h"

namespace hum {

class TracksPane : public juce::Component,
                   public juce::FileDragAndDropTarget,
                   public juce::DragAndDropTarget,
                   private TimelineRuler::Context,
                   private ClipEditorView::Context,
                   private TrackRollView::Context,
                   private SongView::Context {
public:
    explicit TracksPane(TracksHost& host);

    enum class Mode { Song, Track, Clip, Box };
    Mode mode() const { return mode_ == Mode::Song && song_.inBox() ? Mode::Box : mode_; }
    void enterTrackMode(const std::string& node);
    void leaveTrackMode();
    const std::string& trackNode() const { return rollView_.node(); }
    void enterClipMode(const std::string& node, int clipId);
    void leaveClipMode();
    void enterBoxMode(const std::string& node);
    void leaveBoxMode() { song_.leaveBox(); }
    bool inBoxMode() const { return mode() == Mode::Box; }

    SongView& song() { return song_; }
    const SongView& song() const { return song_; }
    ClipEditorView& clipEditor() { return clipView_; }
    TrackRollView& roll() { return rollView_; }

    void rebuild();
    void setPlaybackBeat(double beat);
    void setLiveRecording(bool on);
    void setPixelsPerBeat(double ppb);
    double pixelsPerBeat() const { return view_.ppb; }
    void setScrollBeats(double b);
    double scrollBeats() const { return view_.scrollBeats; }
    double contentEndBeat() const;
    double snapToGrid(double beat) const { return snapBeats(beat, false); }
    bool followPlayback() const { return view_.follow; }
    bool timeSelection(double& from, double& to) const;
    void clearTimeSelection() { view_.sel.active = false; repaint(); }
    std::string selectedNoteNode() const { return song_.selectedNoteNode(); }
    void expandRow(const std::string& node) { song_.expandRow(node); }

    std::function<void(const std::string&)> onSelect;
    std::function<void()> onPatchChanged;
    std::function<void(const std::string&, int)> onOpenClip;
    std::function<void(const std::string&, const std::string&)> onOpenAutomation;
    std::function<void(const std::string&)> onOpenBoxDetail;

    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseMagnify(const juce::MouseEvent&, float scaleFactor) override;

    bool isInterestedInDragSource(const SourceDetails& d) override { return song_.isInterestedInDragSource(d); }
    void itemDragEnter(const SourceDetails&) override { song_.setDropHot(true); }
    void itemDragExit(const SourceDetails&) override { song_.setDropHot(false); }
    void itemDropped(const SourceDetails& d) override { song_.itemDropped(d); }
    bool isInterestedInFileDrag(const juce::StringArray& files) override {
        return SongView::isInterestedInFileDrag(files);
    }
    void fileDragEnter(const juce::StringArray&, int, int) override { song_.setDropHot(true); }
    void fileDragExit(const juce::StringArray&) override { song_.setDropHot(false); }
    void filesDropped(const juce::StringArray& files, int x, int y) override { song_.filesDropped(files, x, y); }

    int loopLaneYForTest() const { return tracksgeo::loopTop() + tracksgeo::kLoopH / 2; }
    int rulerYForTest() const { return tracksgeo::rulerTop() + 3; }
    float beatToXForTest(double beat) const { return view_.beatToX(beat, tracksgeo::kStripW); }
    void setSnapChoiceForTest(double choice) { view_.snapChoice = choice; }
    double snapChoiceForTest() const { return view_.snapChoice; }
    double gridBeatsForTest() const { return gridBeats(); }
    int clickToolForTest(int i) { ruler_.pressToolbar(tracksgeo::toolBox(i).getCentre()); return (int) tool_; }
    juce::Point<int> zoomBoxForTest(int i) const {
        const auto& bar = i / 2 == 0 ? hZoom_ : vZoom_;
        return bar.capBounds(i % 2).getCentre() + bar.getPosition();
    }
    void selectTicksForTest(int from, int to) {
        view_.sel.active = true;
        view_.sel.from = from / (double) Pattern::kTicksPerBeat;
        view_.sel.to = to / (double) Pattern::kTicksPerBeat;
    }

private:
    using Tool = noteedit::Tool;
    static constexpr int kStripW = tracksgeo::kStripW, kTopH = tracksgeo::kTopH;
    static constexpr int kZoomGut = ZoomBar::kGut;
    static constexpr double kSongMaxPpb = 120.0;
    int fieldBottom() const { return getHeight() - kZoomGut; }
    void layoutChildren();
    void traceView(const char* what) const;
    void syncLiveTarget();
    void repaintRecording();
    void stepTrack(int dir);
    double maxPpb() const { return mode_ == Mode::Clip ? tracksgeo::kClipMaxPpb : kSongMaxPpb; }
    double zoomNorm(int axis) const;
    void setZoomNorm(int axis, double t);
    void zoomBy(int axis, double factor);
    void zoomAbout(float x, double factor);

    juce::Rectangle<int> crumbNameBox() const { return {kStripW + 52, 1, 176, kTopH - 2}; }
    juce::Rectangle<int> crumbBackBox() const { return {kStripW + 6, 1, 126, kTopH - 2}; }
    void paintBackCrumb(juce::Graphics&);
    void paintNamedCrumb(juce::Graphics&, const std::string& node, const juce::String& label, float fontH);

    TracksHost& timelineHost() override { return host_; }
    double gridBeats() const override;
    double snapBeats(double beat, bool bypass) const override;
    Tool effectiveTool() const override;
    void selectTool(Tool tool) override;
    bool edgeScroll(juce::Point<int> panePoint) override;
    void beatsChanged(double from, double to) override;
    void viewChanged() override { repaint(); }
    bool showsSongEnd() const override { return mode_ == Mode::Song; }
    void paintCrumb(juce::Graphics& g) override;
    bool crumbDown(const juce::MouseEvent& e, juce::Point<int> p) override;
    juce::String crumbTooltip(juce::Point<int> p) override;
    void showAddTrackMenu(juce::Point<int> screenPos) override { song_.showAddTrackMenu(screenPos); }
    int clipIndexOfId(const std::string& node, int id) const override { return song_.clipIndexOfId(node, id); }
    void leaveClip() override { leaveClipMode(); }
    void patchChanged() override { if (onPatchChanged) onPatchChanged(); }
    void setClipboard(PatternChannel data, int spanTicks) override { song_.setClipboard(std::move(data), spanTicks); }
    bool pasteClipboardInto(const std::string& node, int atTick) override {
        return song_.pasteClipboardInto(node, atTick);
    }
    void rebuildRows() override { rebuild(); }
    void leaveTrack() override { leaveTrackMode(); }
    void stepTrackBy(int dir) override { stepTrack(dir); }
    std::string pinnedRow() const override { return rollView_.node(); }
    void enterTrack(const std::string& node) override { enterTrackMode(node); }
    void enterClip(const std::string& node, int clipId) override { enterClipMode(node, clipId); }
    void liveTargetsChanged() override { syncLiveTarget(); }
    void nodeSelected(const std::string& node) override { if (onSelect) onSelect(node); }
    void openClip(const std::string& node, int clip) override { if (onOpenClip) onOpenClip(node, clip); }
    void openAutomation(const std::string& node, const std::string& param) override {
        if (onOpenAutomation) onOpenAutomation(node, param);
    }
    void openBoxDetail(const std::string& node) override { if (onOpenBoxDetail) onOpenBoxDetail(node); }

    TracksHost& host_;
    TimelineView view_;
    Mode mode_ = Mode::Song;
    Tool tool_ = Tool::Pointer;
    bool liveRec_ = false;
    double recEnd_ = 0.0;
    unsigned lastLocate_ = 0;
    double songPpb_ = 12.0, songScroll_ = 0.0;
    SongView song_{*this, view_};
    TimelineRuler ruler_{*this, view_};
    ClipEditorView clipView_{*this, view_};
    TrackRollView rollView_{*this, view_};
    ZoomBar hZoom_{0, {[] { return true; }, [this] { return zoomNorm(0); },
                       [this](double v) { setZoomNorm(0, v); }, [this](double f) { zoomBy(0, f); }}};
    ZoomBar vZoom_{1, {[this] { return mode() != Mode::Box; }, [this] { return zoomNorm(1); },
                       [this](double v) { setZoomNorm(1, v); }, [this](double f) { zoomBy(1, f); }}};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TracksPane)
};

}
