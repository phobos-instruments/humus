// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/tracks/TracksPane.h"

#include <algorithm>

namespace hum {

void TracksPane::enterTrackMode(const std::string& node) {
    if (node.empty()) return;
    mode_ = Mode::Track;
    rollView_.open(node);
    song_.setVisible(false);
    song_.clearSelections();
    syncLiveTarget();
    repaint();
}

void TracksPane::leaveTrackMode() {
    if (mode_ != Mode::Track) return;
    mode_ = Mode::Song;
    rollView_.close();
    song_.setVisible(true);
    syncLiveTarget();
    song_.rebuildSlots();
    repaint();
}

void TracksPane::stepTrack(int dir) {
    const auto rows = song_.noteRows();
    if (rows.empty()) return;
    int at = 0;
    for (int i = 0; i < (int) rows.size(); ++i)
        if (rows[(size_t) i] == rollView_.node()) at = i;
    enterTrackMode(rows[(size_t) juce::jlimit(0, (int) rows.size() - 1, at + dir)]);
}

void TracksPane::enterClipMode(const std::string& node, int clipId) {
    if (node.empty() || clipId <= 0) return;
    if (mode() == Mode::Song) { songPpb_ = view_.ppb; songScroll_ = view_.scrollBeats; }
    mode_ = Mode::Clip;
    clipView_.open(node, clipId);
    song_.setVisible(false);
    view_.sel.active = false;
    song_.clearSelections();
    syncLiveTarget();
    clipView_.zoomToClip();
}

void TracksPane::leaveClipMode() {
    if (mode_ != Mode::Clip) return;
    mode_ = Mode::Song;
    clipView_.close();
    song_.setVisible(true);
    view_.sel.active = false;
    view_.ppb = songPpb_;
    view_.scrollBeats = songScroll_;
    syncLiveTarget();
    song_.rebuildSlots();
    repaint();
}

void TracksPane::enterBoxMode(const std::string& node) {
    if (mode_ != Mode::Song) return;
    song_.enterBox(node);
}

}
