// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/tracks/TimelineRuler.h"

#include <algorithm>
#include <cmath>
#include <iterator>

#include "gui/common/Localisation.h"
#include "gui/host/EngineHostAutomation.h"
#include "gui/host/TracksHost.h"
#include "gui/pianoroll/NoteEdit.h"
#include "gui/style/LookAndFeel.h"
#include "gui/tracks/TimelineChips.h"
#include "gui/tracks/TimelineGrid.h"
#include "gui/tracks/TimelineTools.h"
#include "gui/tracks/TracksGeometry.h"
#include "hum/Meter.h"
#include "io/PatchDocument.h"

namespace hum {

namespace {

using Tool = noteedit::Tool;
constexpr Tool kToolbar[tracksgeo::kToolCount] = {Tool::Pointer, Tool::Draw, Tool::Line, Tool::Scissors,
                                                  Tool::Eraser};

}

noteedit::Tool TimelineRuler::toolAt(int slot) { return kToolbar[slot]; }

float TimelineRuler::beatToX(double beat) const { return view_.beatToX(beat, tracksgeo::kStripW); }

double TimelineRuler::xToBeat(float x) const { return view_.xToBeat(x, tracksgeo::kStripW); }

bool TimelineRuler::overLoopLane(juce::Point<int> p) const {
    return p.x >= tracksgeo::kStripW && p.y >= tracksgeo::loopTop() && p.y < tracksgeo::rulerTop();
}

void TimelineRuler::paint(juce::Graphics& g) {
    g.fillAll(Palette::background);
    paintBars(g);
    paintTools(g);
    ctx_.paintCrumb(g);
    const bool song = ctx_.showsSongEnd();
    if (const double end = ctx_.timelineHost().songEndBeat(); song && end > 0.0)
        timelinechrome::paintSongEnd(g, beatToX(end), (float) tracksgeo::rulerTop(), (float) getHeight(),
                                     (float) tracksgeo::kStripW, (float) getWidth());
    timelinechrome::paintPlayhead(g, beatToX(view_.playBeat), (float) tracksgeo::rulerTop(),
                                  (float) getHeight(), (float) tracksgeo::kStripW, (float) getWidth(), true);
    if (!song) return;
    g.setColour(Palette::border);
    g.drawVerticalLine(tracksgeo::kStripW - 1, 0.0f, (float) getHeight());
}

void TimelineRuler::paintBars(juce::Graphics& g) {
    g.setColour(Palette::panel);
    g.fillRect(tracksgeo::kStripW, 0, getWidth() - tracksgeo::kStripW, tracksgeo::headerH());

    if (ctx_.timelineHost().automation().loopEnabled()) {
        g.saveState();
        g.reduceClipRegion(tracksgeo::kStripW, 0, getWidth() - tracksgeo::kStripW, tracksgeo::headerH());
        const float a = beatToX(ctx_.timelineHost().automation().loopStartBeat()), b = beatToX(ctx_.timelineHost().automation().loopEndBeat());
        const float top = (float) tracksgeo::loopTop() + 1.0f, h = (float) tracksgeo::kLoopH - 2.0f;
        g.setColour(Palette::accent.withAlpha(alpha::muted));
        g.fillRect(a, top, std::max(2.0f, b - a), h);
        g.setColour(Palette::accent);
        for (const float x : { a, b })
            g.fillRoundedRectangle(x - (float) tracksgeo::kLoopGrip * 0.5f, top, (float) tracksgeo::kLoopGrip, h, 1.5f);
        g.setColour(Palette::background.withAlpha(alpha::mid));
        for (const float x : { a, b })
            g.drawVerticalLine((int) x, top + 1.5f, top + h - 1.5f);
        g.restoreState();
    }
    g.setColour(Palette::border);
    g.drawHorizontalLine(tracksgeo::rulerTop(), (float) tracksgeo::kStripW, (float) getWidth());

    const MeterMap meters = ctx_.timelineHost().automation().meterMap();
    g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 10.5f,
                                juce::Font::plain));
    const auto cb = g.getClipBounds();
    const float barPixels = (float) (meters.at(view_.scrollBeats).quarterNotesPerBar() * view_.ppb);
    const int stride = timelinechrome::barLabelStride(barPixels);
    const double lastBeat = xToBeat((float) std::min(getWidth(), cb.getRight()));
    Meter shown = meters.at(0.0);
    timelinechrome::forEachBar(meters, view_.scrollBeats, lastBeat, [&](double start, double, Meter m) {
        const int bar = meters.barAt(start) - 1;
        const float x = beatToX(start);
        const bool changed = m != shown;
        shown = m;
        if (x < (float) tracksgeo::kStripW || x < (float) cb.getX() - 37.0f) return;
        g.setColour(Palette::border);
        g.drawVerticalLine((int) x, (float) tracksgeo::rulerTop(), (float) tracksgeo::headerH());
        if (bar % stride != 0 && !changed) return;
        juce::String label = timelinechrome::barLabel(bar + 1, start, ctx_.timelineHost().model().clock.tempo,
                                                      barPixels * (float) stride);
        if (changed || (start <= 0.0 && meters.changes().size() > 1)) label << "  " << meterText(m);
        g.setColour(changed ? Palette::accent : Palette::textDim);
        g.drawText(label, (int) x + 3, tracksgeo::rulerTop(), 120, tracksgeo::kRulerH - 2,
                   juce::Justification::centredLeft, false);
    });
}

void TimelineRuler::paintTools(juce::Graphics& g) {
    int i = 0;
    for (const auto t : kToolbar) {
        const auto b = tracksgeo::toolBox(i++);
        const bool on = ctx_.effectiveTool() == t;
        g.setColour(on ? Palette::accent.withAlpha(alpha::scrim) : Palette::panel);
        g.fillRoundedRectangle(b.toFloat(), 3.0f);
        g.setColour(on ? Palette::accent : Palette::border);
        g.drawRoundedRectangle(b.toFloat().reduced(0.5f), 3.0f, 1.0f);
        const auto r = b.toFloat().reduced(6.0f, 5.0f);
        g.setColour(on ? Palette::accent : Palette::textDim);
        timelinechrome::paintToolIcon(g, r, t);
    }
    g.setColour(Palette::panel);
    g.fillRect(0, 0, getWidth(), tracksgeo::kTopH);
    const auto fb = tracksgeo::followBox();
    g.setColour(view_.follow ? Palette::accent : Palette::panelLight);
    g.fillRoundedRectangle(fb.toFloat(), 3.0f);
    g.setColour(view_.follow ? Palette::background : Palette::textDim);
    g.setFont(juce::FontOptions(timelinechrome::kChipFont));
    g.drawText(tr("tracks-pane-paint.follow-play", "Follow Play"), fb, juce::Justification::centred);

    if (view_.sel.active && view_.sel.to > view_.sel.from) {
        const float x0 = juce::jmax((float) tracksgeo::kStripW, beatToX(view_.sel.from));
        const float x1 = beatToX(view_.sel.to);
        if (x1 > x0) {
            g.setColour(Palette::accent.withAlpha(alpha::mist));
            g.fillRect(x0, (float) tracksgeo::rulerTop(), x1 - x0, (float) getHeight());
        }
    }

    const auto ab = tracksgeo::addTrackBox();
    g.setColour(Palette::panel);
    g.fillRoundedRectangle(ab.toFloat(), 3.0f);
    g.setColour(Palette::border);
    g.drawRoundedRectangle(ab.toFloat().reduced(0.5f), 3.0f, 1.0f);
    g.setColour(Palette::accent);
    const auto ac = ab.toFloat().reduced(7.0f, 6.0f);
    g.fillRect(ac.getX(), ac.getCentreY() - 0.75f, ac.getWidth(), 1.5f);
    g.fillRect(ac.getCentreX() - 0.75f, ac.getY(), 1.5f, ac.getHeight());

    timelinechrome::paintSnapChip(g, tracksgeo::snapBox(), ctx_.gridBeats(), view_.snapChoice != 0.0,
                                  view_.snapChoice < 0.0 ? "Free" : nullptr);
}

bool TimelineRuler::pressToolbar(juce::Point<int> p) {
    if (p.x >= tracksgeo::kStripW || p.y >= tracksgeo::headerH()) return false;
    if (tracksgeo::followBox().contains(p)) { view_.follow = !view_.follow; repaint(tracksgeo::followBox()); return true; }
    for (int i = 0; i < tracksgeo::kToolCount; ++i)
        if (tracksgeo::toolBox(i).contains(p)) {
            ctx_.selectTool(kToolbar[i]);
            return true;
        }
    if (tracksgeo::snapBox().contains(p)) { showSnapMenu(localPointToGlobal(p)); return true; }
    if (tracksgeo::addTrackBox().contains(p)) {
        ctx_.showAddTrackMenu(localPointToGlobal(p));
        return true;
    }
    return false;
}

void TimelineRuler::pressRuler(const juce::MouseEvent& e, juce::Point<int> p) {
    const double b = std::max(0.0, xToBeat((float) p.x));
    if (p.y < tracksgeo::loopTop()) return;
    if (p.y < tracksgeo::rulerTop()) {
        if (e.mods.isPopupMenu()) { showLoopMenu(p); return; }
        const double ls = ctx_.timelineHost().automation().loopStartBeat(), le = ctx_.timelineHost().automation().loopEndBeat();
        const float xs = beatToX(ls), xe = beatToX(le);
        const bool on = ctx_.timelineHost().automation().loopEnabled();
        if (on && std::abs(p.x - xs) <= tracksgeo::kLoopHit) {
            drag_ = Drag::LoopL; loopOrigStart_ = ls; loopOrigEnd_ = le;
        } else if (on && std::abs(p.x - xe) <= tracksgeo::kLoopHit) {
            drag_ = Drag::LoopR; loopOrigStart_ = ls; loopOrigEnd_ = le;
        } else if (on && p.x > xs && p.x < xe) {
            drag_ = Drag::LoopMove; loopAnchor_ = b - ls;
            loopOrigStart_ = ls; loopOrigEnd_ = le;
            setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        } else {
            drag_ = Drag::LoopNew; loopAnchor_ = ctx_.snapBeats(b, e.mods.isAltDown());
            loopDrawn_ = false;
        }
    } else if (ctx_.timelineHost().songEndBeat() > 0.0
               && std::abs(p.x - beatToX(ctx_.timelineHost().songEndBeat())) <= 7) {
        drag_ = Drag::SongEnd;
        ctx_.timelineHost().pushUndo();
    } else if (e.mods.isShiftDown()) {
        drag_ = Drag::TimeSelect;
        const double oldFrom = view_.sel.from, oldTo = view_.sel.to;
        const bool wasActive = view_.sel.active;
        view_.sel.anchor = ctx_.snapBeats(b, e.mods.isAltDown());
        view_.sel.from = view_.sel.to = view_.sel.anchor;
        view_.sel.active = true;
        view_.sel.dragging = true;
        selectionChanged(oldFrom, oldTo, wasActive);
    } else {
        drag_ = Drag::Scrub;
        const bool wasActive = view_.sel.active;
        view_.sel.active = false;
        ctx_.timelineHost().setPositionBeats(ctx_.snapBeats(b, e.mods.isAltDown()));
        if (wasActive) ctx_.beatsChanged(view_.sel.from, view_.sel.to);
    }
    return;
}

void TimelineRuler::mouseDown(const juce::MouseEvent& e) {
    const auto p = e.getPosition();
    if (p.y < tracksgeo::kTopH && ctx_.crumbDown(e, p)) return;
    if (pressToolbar(p)) return;
    if (p.x >= tracksgeo::kStripW) pressRuler(e, p);
}

void TimelineRuler::mouseDrag(const juce::MouseEvent& e) {
    if (drag_ == Drag::None) return;
    if (!sync_) sync_.emplace(ctx_.timelineHost());
    ctx_.edgeScroll(e.getPosition());
    auto& host = ctx_.timelineHost();
    const double beat = std::max(0.0, xToBeat((float) e.getPosition().x));
    const bool alt = e.mods.isAltDown();
    switch (drag_) {
        case Drag::Scrub:
            host.setPositionBeats(ctx_.snapBeats(beat, alt));
            break;
        case Drag::SongEnd: {
            const double was = host.songEndBeat();
            host.setSongLengthBeats(ctx_.snapBeats(beat, alt));
            ctx_.beatsChanged(was, host.songEndBeat());
            break;
        }
        case Drag::LoopNew: {
            const double b = ctx_.snapBeats(beat, alt);
            loopDrawn_ = loopDrawn_ || std::abs(b - loopAnchor_) > 1e-6;
            host.automation().setLoop(std::min(loopAnchor_, b), std::max(loopAnchor_, b),
                                      std::abs(b - loopAnchor_) > 1e-6);
            repaintLoopLane();
            break;
        }
        case Drag::LoopL:
            host.automation().setLoop(std::min(ctx_.snapBeats(beat, alt), loopOrigEnd_ - 1e-3),
                                      loopOrigEnd_, true);
            repaintLoopLane();
            break;
        case Drag::LoopR:
            host.automation().setLoop(loopOrigStart_,
                                      std::max(ctx_.snapBeats(beat, alt), loopOrigStart_ + 1e-3), true);
            repaintLoopLane();
            break;
        case Drag::LoopMove: {
            const double len = loopOrigEnd_ - loopOrigStart_;
            const double s = ctx_.snapBeats(beat - loopAnchor_, alt);
            host.automation().setLoop(std::max(0.0, s), std::max(0.0, s) + len, true);
            repaintLoopLane();
            break;
        }
        case Drag::TimeSelect: {
            const double oldFrom = view_.sel.from, oldTo = view_.sel.to;
            const double b = ctx_.snapBeats(std::max(0.0, xToBeat((float) e.x)), alt);
            view_.sel.from = std::min(view_.sel.anchor, b);
            view_.sel.to = std::max(view_.sel.anchor, b);
            selectionChanged(oldFrom, oldTo, true);
            break;
        }
        case Drag::None: break;
    }
}

void TimelineRuler::selectionChanged(double oldFrom, double oldTo, bool wasActive) {
    double a = view_.sel.from, b = view_.sel.to;
    if (wasActive) {
        a = std::min(a, oldFrom);
        b = std::max(b, oldTo);
    }
    ctx_.beatsChanged(a, b);
}

void TimelineRuler::repaintLoopLane() {
    repaint(tracksgeo::kStripW, tracksgeo::loopTop(), getWidth() - tracksgeo::kStripW, tracksgeo::kLoopH);
}

void TimelineRuler::mouseUp(const juce::MouseEvent&) {
    sync_.reset();
    if (drag_ == Drag::None) return;
    auto& host = ctx_.timelineHost();
    if (drag_ == Drag::LoopNew && !loopDrawn_ && host.automation().loopEnabled())
        host.automation().setLoop(host.automation().loopStartBeat(), host.automation().loopEndBeat(), false);
    if (drag_ == Drag::LoopNew) repaintLoopLane();
    view_.sel.dragging = false;
    drag_ = Drag::None;
}

void TimelineRuler::mouseMove(const juce::MouseEvent& e) {
    const auto p = e.getPosition();
    hover_ = p;
    if (!overLoopLane(p)) {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        return;
    }
    auto& host = ctx_.timelineHost();
    const float xs = beatToX(host.automation().loopStartBeat());
    const float xe = beatToX(host.automation().loopEndBeat());
    const bool on = host.automation().loopEnabled();
    setMouseCursor(on && (std::abs(p.x - xs) <= tracksgeo::kLoopHit || std::abs(p.x - xe) <= tracksgeo::kLoopHit)
                       ? juce::MouseCursor::LeftRightResizeCursor
                   : on && p.x > xs && p.x < xe ? juce::MouseCursor::DraggingHandCursor
                                                : juce::MouseCursor::CrosshairCursor);
}

void TimelineRuler::mouseDoubleClick(const juce::MouseEvent& e) {
    if (!overLoopLane(e.getPosition())) return;
    auto& host = ctx_.timelineHost();
    if (host.automation().loopEnabled())
        host.automation().setLoop(host.automation().loopStartBeat(), host.automation().loopEndBeat(), false);
    repaintLoopLane();
}

juce::String TimelineRuler::getTooltip() {
    const auto p = hover_;
    if (p.x < tracksgeo::kStripW) {
        static const char* names[] = {
            "Pointer (1) - select, move, resize; drag a lane to select points",
            "Pencil (2) - draw notes, clips and freehand automation",
            "Line (3) - rule a straight automation segment",
            "Scissors (4) - split at the click",
            "Eraser (5) - sweep to delete",
        };
        for (int i = 0; i < tracksgeo::kToolCount; ++i)
            if (tracksgeo::toolBox(i).contains(p)) return names[i];
        if (tracksgeo::addTrackBox().contains(p)) return tr("tracks-pane-roll.add-a-track-wired", "Add a track, wired");
        if (tracksgeo::snapBox().contains(p))
            return view_.snapChoice < 0.0 ? "Snap: free - click to choose a grid"
                 : view_.snapChoice > 0.0 ? "Snap: chosen - click to change or follow the zoom"
                                     : "Snap follows the zoom - click to choose one";
        if (tracksgeo::followBox().contains(p)) return tr("tracks-pane-roll.follow-the-playhead-while-it", "Follow the playhead while it plays");
    }
    if (overLoopLane(p))
        return ctx_.timelineHost().automation().loopEnabled()
            ? "Loop - drag the ends to trim, the body to move; click outside or double-click to remove"
            : "Drag to set a loop - right-click to loop the selection";
    return ctx_.crumbTooltip(p);
}

void TimelineRuler::showSnapMenu(juce::Point<int> sp) {
    const double bar = juce::jmax(1, ctx_.timelineHost().automation().timeSigNumerator());
    struct Item { const char* key; const char* name; double beats; };
    const Item items[] = {{"tracks-pane.snap-auto", "Auto (follows the zoom)", 0.0},
                          {"tracks-pane.snap-free", "Free", -1.0},
                          {"tracks-pane.snap-bar", "Bar", bar},
                          {"tracks-pane.snap-half", "1/2", 2.0},
                          {"tracks-pane.snap-quarter", "1/4", 1.0},
                          {"tracks-pane.snap-eighth", "1/8", 0.5},
                          {"tracks-pane.snap-sixteenth", "1/16", 0.25},
                          {"tracks-pane.snap-thirty-second", "1/32", 0.125},
                          {"tracks-pane.snap-sixty-fourth", "1/64", 0.0625}};
    juce::PopupMenu m;
    for (int i = 0; i < (int) std::size(items); ++i)
        m.addItem(i + 1, tr(items[i].key, items[i].name), true, std::abs(view_.snapChoice - items[i].beats) < 1e-9);
    m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({sp.x, sp.y, 1, 1}),
                    [this, bar](int r) {
        const double beats[] = {0.0, -1.0, bar, 2.0, 1.0, 0.5, 0.25, 0.125, 0.0625};
        if (r >= 1 && r <= 9) { view_.snapChoice = beats[r - 1]; ctx_.viewChanged(); }
    });
}

void TimelineRuler::showLoopMenu(juce::Point<int> at) {
    juce::PopupMenu m;
    double from = 0.0, to = 0.0;
    if (view_.sel.active && view_.sel.to > view_.sel.from) { from = view_.sel.from; to = view_.sel.to; }
    const bool selected = to > from;
    m.addItem(1, tr("tracks-pane-input.remove-loop", "Remove loop"), ctx_.timelineHost().automation().loopEnabled());
    m.addItem(2, tr("tracks-pane-input.loop-the-selection", "Loop the selection"), selected);
    m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(
                        juce::Rectangle<int>(localPointToGlobal(at), localPointToGlobal(at))),
                    [this, from, to](int r) {
        if (r == 1)
            ctx_.timelineHost().automation().setLoop(ctx_.timelineHost().automation().loopStartBeat(),
                                       ctx_.timelineHost().automation().loopEndBeat(), false);
        else if (r == 2)
            ctx_.timelineHost().automation().setLoop(from, to, true);
        repaintLoopLane();
    });
}

}
