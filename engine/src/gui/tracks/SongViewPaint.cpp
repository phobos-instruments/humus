// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/style/Colours.h"
#include "gui/common/PerfLog.h"
#include "gui/tracks/SongView.h"
#include "gui/host/EngineHostRecord.h"
#include "hum/Organism.h"
#include "hum/caps/Files.h"


#include <climits>
#include <cmath>
#include <cstdint>

#include "gui/style/EnvelopePainter.h"
#include "gui/style/LookAndFeel.h"
#include "gui/editor/OrganismEditor.h"
#include "gui/tracks/CutGuide.h"
#include "gui/tracks/TimelineGrid.h"
#include "gui/tracks/WaveformCache.h"
#include "gui/common/Localisation.h"


namespace hum {

void SongView::paintDropHint(juce::Graphics& g) {
    g.setColour(Palette::accent.withAlpha(alpha::mist));
    g.fillRect(getBounds());
    g.setColour(Palette::accent);
    g.drawRect(getBounds().reduced(2), 2);
}

void SongView::paintTimeSelection(juce::Graphics& g) {
    if (!view_.sel.active || view_.sel.to <= view_.sel.from) return;
    const float x0 = juce::jmax((float) kStripW, beatToX(view_.sel.from));
    const float x1 = beatToX(view_.sel.to);
    if (x1 <= x0) return;
    g.setColour(Palette::accent.withAlpha(alpha::mist));
    g.fillRect(x0, (float) rulerTop(), x1 - x0, (float) getBottom());
}

void SongView::paint(juce::Graphics& g) {
    perf::Scope scope("tracks.paint");
    g.setOrigin(-getPosition());
    paintField(g);
}

void SongView::paintField(juce::Graphics& g) {
    g.fillAll(Palette::background);
    paintTimeSelection(g);

    if (rows_.empty()) {
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(14.0f));
        auto r = getBounds();
        g.drawText(tr("tracks-pane-paint.drop-media-files-here", "Drop media files here"), r.removeFromTop(r.getHeight() / 2 + 10),
                   juce::Justification::centredBottom);
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(12.0f));
        g.drawText(tr("tracks-pane-paint.or-press-to-add-an", "... or press + to add an audio, MIDI or video track"),
                   r, juce::Justification::centredTop);
        if (dropHot_) paintDropHint(g);
    } else {
        if (dropHot_) paintDropHint(g);
        const auto cb = g.getClipBounds();
        g.saveState();
        g.reduceClipRegion(getBounds());
        for (const auto& s : slots_) {
            if (s.y + s.h < cb.getY() || s.y > cb.getBottom()) continue;
            if (s.kind == trackslayout::Kind::Track)          paintRow(g, s.track);
            else if (s.kind == trackslayout::Kind::PodHeader) paintPodHeader(g, s);
            else if (s.kind == trackslayout::Kind::BoxRow)    paintBoxRow(g, s);
            else                                              paintAutoLane(g, s);
        }
        g.restoreState();
    }

    if (drag_ == Drag::ClipMarquee && !clipMarquee_.isEmpty()) {
        g.setColour(Palette::accent.withAlpha(alpha::mist));
        g.fillRect(clipMarquee_);
        g.setColour(Palette::accent);
        g.drawRect(clipMarquee_, 1);
    }
    paintLinePreview(g);
    if (cutguide::active(ctx_, hover_)) cutguide::paint(g, cutguide::x(view_, ctx_, hover_.x), getBottom());
    paintPointSelection(g);

    if (const double end = host().songEndBeat(); end > 0.0)
        timelinechrome::paintSongEnd(g, beatToX(end), (float) rulerTop(),
                                     (float) getBottom(), (float) kStripW, (float) getWidth());

    timelinechrome::paintPlayhead(g, beatToX(view_.playBeat), (float) rulerTop(),
                                  (float) getBottom(), (float) kStripW,
                                  (float) getWidth(), true);

    g.setColour(Palette::border);
    g.drawVerticalLine(kStripW - 1, 0.0f, (float) getBottom());
}

void SongView::setDropHot(bool hot) {
    dropHot_ = hot;
    repaint();
}

void SongView::repaintCutGuide() { cutguide::repaintMove(*this, view_, ctx_, hover_.x, hover_.x); }

void SongView::repaintRow(int row) {
    for (const auto& s : slots_)
        if (s.track == row && s.kind != trackslayout::Kind::PodHeader) repaintPane({0, s.y, getWidth(), s.h});
}

double SongView::takeStartBeat(const std::string& node) const {
    if (auto* cr = dynamic_cast<ClipRecorder*>(host().liveOrganism(node));
        cr != nullptr && cr->takeActive() && cr->takeLengthSamples() > 0)
        return cr->takeStartBeat();
    return host().record().videoTakeStartBeat(node);
}

bool SongView::rowRecording(int row, const std::vector<std::string>& live) const {
    const auto& node = rows_[(size_t) row];
    if (std::find(live.begin(), live.end(), node) != live.end()) return true;
    if (host().midi().isRecordTarget(node) || takeStartBeat(node) >= 0.0) return true;
    if (const auto* cm = host().model().byName(node); cm != nullptr && host().nodeRecordsMedia(node))
        for (const auto& p : cm->properties)
            if (p.name == "Record") return p.value >= 0.5;
    return false;
}

void SongView::repaintRecordingRows() {
    if (!isVisible()) return;
    const auto live = liveTargets();
    for (int row = 0; row < (int) rows_.size(); ++row)
        if (rowRecording(row, live)) repaintRow(row);
}

}
