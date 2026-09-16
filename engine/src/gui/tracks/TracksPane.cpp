// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/tracks/TracksPane.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "gui/style/LookAndFeel.h"

namespace hum {

TracksPane::TracksPane(TracksHost& host) : host_(host) {
    setBufferedToImage(true);
    setOpaque(true);
    setWantsKeyboardFocus(true);
    addAndMakeVisible(song_);
    addChildComponent(clipView_);
    addChildComponent(rollView_);
    addAndMakeVisible(ruler_);
    addAndMakeVisible(hZoom_);
    addAndMakeVisible(vZoom_);
}

void TracksPane::traceView(const char* what) const {
    static const bool on = std::getenv("HUMUS_VIEW_DEBUG") != nullptr;
    if (!on) return;
    std::fprintf(stderr, "[view] %-12s rows=%d scroll=%.3f v=%d size=%dx%d ppb=%.2f mode=%d "
                         "follow=%d live=%d\n",
                 what, song_.rowCount(), view_.scrollBeats, view_.vScroll, getWidth(), getHeight(),
                 view_.ppb, (int) mode(), (int) view_.follow, (int) liveRec_);
}

void TracksPane::rebuild() {
    traceView("rebuild");
    song_.rebuild();
    if (mode_ == Mode::Clip && clipView_.clipOrdinal() < 0) leaveClipMode();
}

void TracksPane::layoutChildren() {
    const int h = tracksgeo::headerH();
    ruler_.setBounds(0, 0, getWidth(), h);
    song_.setBounds(0, h, getWidth(), getHeight() - h);
    clipView_.setBounds(0, h, getWidth(), fieldBottom() - h);
    rollView_.setBounds(0, h, getWidth(), fieldBottom() - h);
    hZoom_.setBounds(0, fieldBottom(), getWidth(), kZoomGut);
    vZoom_.setBounds(getWidth() - kZoomGut, h, kZoomGut, fieldBottom() - h);
}

void TracksPane::resized() {
    traceView("resized");
    layoutChildren();
}

void TracksPane::paint(juce::Graphics& g) { g.fillAll(Palette::background); }

bool TracksPane::timeSelection(double& from, double& to) const {
    if (!view_.sel.active || view_.sel.to <= view_.sel.from) return false;
    from = view_.sel.from; to = view_.sel.to;
    return true;
}

double TracksPane::contentEndBeat() const {
    if (mode_ == Mode::Clip) {
        ClipEditor::ClipInfo ci;
        if (clipView_.clipInfo(ci))
            return (ci.startTick + ci.lengthTicks) / (double) Pattern::kTicksPerBeat;
    }
    return host_.songEndBeat();
}

void TracksPane::setPlaybackBeat(double beat) {
    const bool located = host_.locateStamp() != lastLocate_;
    lastLocate_ = host_.locateStamp();
    if (located && isVisible() && !song_.dragging()) {
        const double target = host_.locateBeat();
        if (view_.beatToX(target, kStripW) > (float) getWidth() || target < view_.scrollBeats) {
            traceView("locate-jump");
            view_.scrollBeats = std::max(0.0, target - 4.0);
            view_.playBeat = beat;
            repaint();
            return;
        }
    }
    if (std::abs(beat - view_.playBeat) < 1e-4) return;
    const float oldX = view_.beatToX(view_.playBeat, kStripW);
    view_.playBeat = beat;
    if (!isVisible()) return;
    const bool offScreen = view_.beatToX(beat, kStripW) > (float) getWidth() || beat < view_.scrollBeats;
    if (offScreen && !song_.dragging() && (view_.follow || !host_.isPlaying())) {
        traceView("follow-jump");
        view_.scrollBeats = std::max(0.0, beat - 4.0);
        repaint();
        return;
    }
    if (liveRec_ && mode_ != Mode::Song) { repaint(); return; }
    const float newX = view_.beatToX(beat, kStripW);
    repaint((int) std::floor(juce::jmin(oldX, newX)) - 6, 0,
            (int) std::ceil(std::abs(newX - oldX)) + 12, getHeight());
    if (liveRec_) repaintRecording();
}

void TracksPane::setLiveRecording(bool on) {
    if (on && !liveRec_) recEnd_ = host_.songEndBeat();
    liveRec_ = on;
}

void TracksPane::repaintRecording() {
    song_.repaintRecordingRows();
    if (const double end = host_.songEndBeat(); end != recEnd_) {
        beatsChanged(recEnd_, end);
        recEnd_ = end;
    }
}

void TracksPane::setScrollBeats(double b) {
    traceView("scroll");
    view_.scrollBeats = std::max(0.0, b);
    repaint();
}

void TracksPane::setPixelsPerBeat(double ppb) {
    view_.ppb = juce::jlimit(2.0, maxPpb(), ppb);
    song_.rebuildSlots();
    repaint();
}

double TracksPane::gridBeats() const {
    if (view_.snapChoice > 0.0) return view_.snapChoice;
    return trackslayout::gridBeats(view_.ppb, host_.automation().meterAt(host_.positionBeats()));
}

double TracksPane::snapBeats(double beat, bool bypass) const {
    if (bypass || view_.snapChoice < 0.0) return std::max(0.0, beat);
    if (view_.snapChoice > 0.0) return std::max(0.0, std::round(beat / view_.snapChoice) * view_.snapChoice);
    const MeterMap meters = host_.automation().meterMap();
    return trackslayout::snapBeats(beat, view_.ppb, meters.at(std::max(0.0, beat)),
                                   meters.barStartBefore(std::max(0.0, beat)), false);
}

void TracksPane::beatsChanged(double from, double to) {
    const float x0 = view_.beatToX(std::min(from, to), kStripW);
    const float x1 = view_.beatToX(std::max(from, to), kStripW);
    repaint((int) std::floor(x0) - 10, 0, (int) std::ceil(x1 - x0) + 20, getHeight());
}

void TracksPane::syncLiveTarget() {
    std::vector<std::string> t;
    if (mode_ == Mode::Track) {
        if (!rollView_.node().empty()) t.push_back(rollView_.node());
    } else {
        t = song_.liveTargets();
    }
    host_.setLiveTargets(std::move(t));
}

}
