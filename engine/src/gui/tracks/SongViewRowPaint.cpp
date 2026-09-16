// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/AutomateMenu.h"
#include "gui/style/Colours.h"
#include "gui/common/PerfLog.h"
#include "gui/tracks/SongView.h"
#include "gui/host/EngineHostRecord.h"
#include "core/timeline/ClipOps.h"
#include "hum/ClipStack.h"
#include <climits>
#include <cmath>
#include <cstdint>
#include "core/packs/Categories.h"
#include "core/packs/Roles.h"
#include "gui/tracks/ClipColors.h"
#include "gui/style/EnvelopePainter.h"
#include "gui/style/LookAndFeel.h"
#include "gui/editor/OrganismEditor.h"
#include "gui/tracks/ClipChrome.h"
#include "gui/tracks/NotePlot.h"
#include "gui/tracks/TimelineChips.h"
#include "gui/tracks/TimelineGrid.h"
#include "gui/tracks/WaveformCache.h"
#include "gui/common/Localisation.h"
#include "hum/dsp/DspMath.h"

namespace hum {

void SongView::paintRow(juce::Graphics& g, int row) {
    const auto& node = rows_[(size_t) row];
    const int y = rowTop(row);
    const auto cb = g.getClipBounds();

    if (cb.getX() < kStripW) paintRowHeader(g, row, y);

    g.setColour(Palette::background.brighter(row % 2 ? 0.02f : 0.05f));
    g.fillRect(kStripW, y, getWidth() - kStripW, rowH_ - 1);
    timelinechrome::paintTimeGrid(g, {kStripW, y, getWidth() - kStripW, rowH_ - 1}, kStripW,
                                  view_.scrollBeats, view_.ppb, host().automation().meterMap(), view_.snapChoice);
    const juce::Colour laneCol = timelinechrome::laneAccent(host().model(), node);
    bool quiet = host().bypassed(node);
    if (nodeMuted(node)) quiet = true;
    auto dim = [quiet](juce::Colour c) {
        return quiet ? c.withSaturation(c.getSaturation() * 0.25f).withAlpha(alpha::strong) : c;
    };
    auto cCol = [&](int idx) { return dim(idx > 0 ? clipColour(idx) : laneCol); };
    auto cFill = [&](int idx) { return dim(idx > 0 ? clipFill(idx) : laneCol.darker(0.72f)); };

    for (const auto& ci : host().clips().list(node)) {
        auto b = clipBounds(row, ci);
        if (b.getRight() < kStripW || b.getX() > getWidth()) {
            if (!ci.looped) continue;
        }
        if (!ci.looped && (b.getRight() < cb.getX() || b.getX() > cb.getRight()))
            continue;
        const int unclampedLeft = b.getX();
        b.setLeft(std::max(b.getX(), kStripW));
        if (b.getWidth() > 0 && b.getRight() >= cb.getX() && b.getX() <= cb.getRight()) {
            const bool sel = (row == selClipRow_ && ci.index == selClip_)
                             || clipSelected(row, ci.id);
            g.setColour(sel ? cFill(ci.color).brighter(0.4f) : cFill(ci.color));
            g.fillRoundedRectangle(b.toFloat(), 3.0f);
            if (ci.isAudio)      paintWaveform(g, b, unclampedLeft, ci, cCol(ci.color));
            else if (showsFilmstrip(ci)) paintFilmstrip(g, b, ci, cCol(ci.color));
            else                 paintNotes(g, b, unclampedLeft, node, ci, cCol(ci.color));
            if (sel) {
                g.setColour(Palette::text.withAlpha(alpha::veil));
                g.fillRoundedRectangle(b.toFloat(), 3.0f);
                g.setColour(Palette::text.withAlpha(alpha::nearOpaque));
                g.drawRoundedRectangle(b.toFloat().reduced(1.0f), 3.0f, 2.0f);
            } else {
                g.setColour(cCol(ci.color));
                g.drawRoundedRectangle(b.toFloat(), 3.0f, 1.2f);
            }
            g.setColour(Palette::text);
            g.setFont(juce::FontOptions(10.0f));
            const auto label = ci.name.empty()
                ? juce::String("clip ") + juce::String(ci.index + 1) : juce::String(ci.name);
            const bool marks = unclampedLeft == b.getX() && b.getWidth() > 3 * kFadeGrip;
            if (b.getWidth() > 30)
                g.drawText(label, b.reduced(4, 2).withTrimmedLeft(marks ? kFadeGrip : 0),
                           juce::Justification::topLeft, true);
            if (ci.hasMedia()) {
                timelinechrome::paintFades(g, b, ci.fadeInTicks, ci.fadeOutTicks,
                                           ci.lengthTicks, cCol(ci.color),
                                           ci.fadeInCurve, ci.fadeOutCurve);
                if (marks)
                    timelinechrome::paintFadeGrips(g, b, kFadeGrip, ci.fadeInTicks,
                                                   ci.fadeOutTicks, cCol(ci.color));
                timelinechrome::paintFadeCurveGrips(g, b, ci.fadeInTicks, ci.fadeOutTicks,
                                                    ci.lengthTicks, ci.fadeInCurve,
                                                    ci.fadeOutCurve, cCol(ci.color));
            }
            if (!ci.looped)
                timelinechrome::paintRepeatGrip(g, b, kFadeGrip, cCol(ci.color), sel);
            if (ci.hasMedia())
                timelinechrome::paintWarpBadge(g, b, ci.sourceBpm, host().tempo(),
                                               ci.warpMode, cCol(ci.color));
        }

        if (ci.looped && ci.lengthTicks > 0) {
            int ghostEnd = ci.startTick;
            if (const auto* cm = host().model().byName(node)) {
                const auto chans = clipops::clipChannels(cm->pattern);
                if (ci.index < (int) chans.size()) {
                    const auto sp = clipstack::soundingSpans(cm->pattern,
                                                             chans[(size_t) ci.index]);
                    if (!sp.empty())
                        ghostEnd = sp.back().startTick + sp.back().lengthTicks;
                }
            }
            g.setColour(cFill(ci.color).withAlpha(alpha::muted));
            for (int start = ci.startTick + ci.lengthTicks; start < ghostEnd;
                 start += ci.lengthTicks) {
                const float gx0 = tickToX(start);
                const float gx1 = tickToX(std::min(start + ci.lengthTicks, ghostEnd));
                if (gx0 > (float) getWidth() || gx0 > (float) cb.getRight()) break;
                if (gx1 < (float) kStripW || gx1 < (float) cb.getX()) continue;
                g.fillRoundedRectangle(std::max(gx0, (float) kStripW), (float) b.getY(),
                                       std::max(4.0f, gx1 - std::max(gx0, (float) kStripW)),
                                       (float) b.getHeight(), 3.0f);
            }
        }
    }

    if (!wantsBoxRow(row)) paintBoxes(g, row);

    if (const double takeStart = takeStartBeat(node); takeStart >= 0.0) {
        const float x0 = std::max(beatToX(takeStart), (float) kStripW);
        const float x1 = std::max(beatToX(view_.playBeat), x0 + 4.0f);
        const juce::Rectangle<float> r(x0, (float) y + 2.0f, x1 - x0, (float) rowH_ - 5.0f);
        g.setColour(ink::state::armed.withAlpha(alpha::muted));
        g.fillRoundedRectangle(r, 3.0f);
        g.setColour(ink::state::armed);
        g.drawRoundedRectangle(r, 3.0f, 1.0f);
        g.setFont(juce::FontOptions(11.0f));
        g.drawText(tr("tracks-pane-paint.rec", "REC"), r.reduced(4.0f, 0.0f).toNearestInt(), juce::Justification::centredLeft);
    }
}

void SongView::paintRowHeader(juce::Graphics& g, int row, int y) {
    const auto& node = rows_[(size_t) row];
    g.setColour(Palette::panel);
    g.fillRect(0, y, kStripW, rowH_ - 1);
    if (row == selClipRow_ || selTracks_.count(node) != 0) {
        const auto ac = timelinechrome::laneAccent(host().model(), node);
        g.setColour(ac.withAlpha(alpha::veil));
        g.fillRect(0, y, kStripW - 1, rowH_ - 1);
        g.setColour(ac);
        g.fillRect(0, y, 3, rowH_ - 1);
    }
    int nameX = 6;
    if (hasLanes(row) && !wantsBoxRow(row)) {
        const auto fb = foldBox(row).toFloat();
        const bool open = expanded_.count(node) != 0;
        juce::Path tri;
        if (open) tri.addTriangle(fb.getX() + 2, fb.getY() + 4, fb.getRight() - 2, fb.getY() + 4,
                                  fb.getCentreX(), fb.getBottom() - 3);
        else      tri.addTriangle(fb.getX() + 4, fb.getY() + 2, fb.getX() + 4, fb.getBottom() - 2,
                                  fb.getRight() - 3, fb.getCentreY());
        g.setColour(Palette::textDim);
        g.fillPath(tri);
        nameX = 20;
    }
    g.setColour(timelinechrome::laneAccent(host().model(), node));
    g.fillRoundedRectangle((float) nameX, (float) y + 7.0f, 8.0f, 8.0f, 2.0f);
    nameX += 12;

    g.setColour(Palette::text);
    g.setFont(juce::FontOptions(12.0f));
    const auto* ncm = host().model().byName(node);
    const auto pseudo = ncm != nullptr ? pseudoOwnerLabel(ncm->displayClass) : std::string();
    juce::String shown = pseudo.empty() ? juce::String(node) : juce::String(pseudo);
    if (pseudo.empty())
        if (const auto pod = podOfRow(row); !pod.empty())
            shown = juce::String(node.substr(pod.size() + 1));
    g.drawText(shown, nameX, y + 2, kStripW - nameX - 82, 18,
               juce::Justification::centredLeft, true);
    const bool rowHeld = host().automation().anyHeld(node);
    if (ncm != nullptr && classHasRole(ncm->classRaw, role::kMidiTrack))
        timelinechrome::paintMidiDestChip(
            g, {nameX, y + 18, kStripW - nameX - (rowHeld ? 30 : 8), 17},
            host().model(), node);
    else
        timelinechrome::paintDestChip(
            g, {nameX, y + 21, kStripW - nameX - (rowHeld ? 30 : 8), 13},
            host().model(), node);
    if (rowHeld) timelinechrome::paintHeldBadge(g, heldBox(row));

    const bool muted = nodeMuted(node);
    bool recParam = false;
    if (const auto* cm = host().model().byName(node))
        for (const auto& p : cm->properties)
            if (p.name == "Record") { recParam = p.value >= 0.5; break; }
    const bool armed = host().nodeRecordsMedia(node) ? recParam
                                                    : host().midi().isRecordTarget(node);
    auto box = [&](juce::Rectangle<int> r, const char* t, bool on, juce::Colour onCol) {
        g.setColour(on ? onCol : Palette::panelLight);
        g.fillRect(r);
        g.setColour(Palette::border);
        g.drawRect(r, 1);
        g.setColour(on ? Palette::background : Palette::textDim);
        g.setFont(juce::FontOptions(10.0f));
        g.drawText(t, r, juce::Justification::centred, false);
    };
    box(muteBox(row), "M", muted, Palette::accent);
    if (arrangeable_.count(node) != 0)
        box(soloBox(row), "S", host().soloed(node), Palette::warnAmber());
    box(recBox(row), "R", armed, ink::state::armed);
}

void SongView::paintAutoLane(juce::Graphics& g, const trackslayout::Slot& slot) {
    const auto& node = rows_[(size_t) slot.track];
    const auto* cm = host().model().byName(node);
    if (!cm) return;
    const hum::AutomationLane* lane = nullptr;
    for (const auto& l : cm->automation)
        if (l.propertyName == slot.param) { lane = &l; break; }
    const int y = slot.y, h = slot.h;

    g.setColour(Palette::panel.darker(0.15f));
    g.fillRect(0, y, kStripW, h - 1);
    g.setColour(Palette::border.withAlpha(alpha::dim));
    g.drawHorizontalLine(y, 0.0f, (float) getWidth());
    timelinechrome::paintTimeGrid(g, {kStripW, y + 1, getWidth() - kStripW, h - 2}, kStripW,
                                  view_.scrollBeats, view_.ppb, host().automation().meterMap(), view_.snapChoice);
    g.setColour(Palette::textDim);
    g.setFont(juce::FontOptions(11.0f));
    g.drawText(juce::String::fromUTF8("\xe2\x86\xb3 ") + slot.param, 20, y, kStripW - 76, h,
               juce::Justification::centredLeft, true);
    const bool muted = lane && lane->mute;
    auto box = [&](juce::Rectangle<int> r, const char* t, bool on, juce::Colour onCol) {
        g.setColour(on ? onCol : Palette::panelLight);
        g.fillRect(r);
        g.setColour(on ? Palette::background : Palette::textDim);
        g.setFont(juce::FontOptions(9.0f));
        g.drawText(t, r, juce::Justification::centred);
    };
    box({kStripW - 54, y + (h - 12) / 2, 22, 12}, "M", muted, Palette::accent);
    const bool held = host().automation().isHeld(node, slot.param);
    if (held) timelinechrome::paintHeldBadge(g, heldLaneBox(slot));

    if (!lane || lane->points.empty()) return;
    const auto range = envpaint::laneRange(cm, slot.param,
                                           (int) host().model().metapad.snapshots.size());
    const double lo = range.first, hi = range.second;
    const double span = hi > lo ? hi - lo : 1.0;
    g.saveState();
    g.reduceClipRegion(kStripW, y, getWidth() - kStripW, h);
    envpaint::draw(g, lane->points, lane->kind, (float) kStripW, (float) y, (float) h,
                   [this](double b) { return beatToX(b); },
                   [=](double v) { return (float) (y + h - 3 - (v - lo) / span * (h - 6)); },
                   muted ? Palette::textDim : (held ? Palette::warnAmber() : Palette::accent),
                   2.5f);
    const int me = (int) (&slot - slots_.data());
    const bool dragging = dragAutoPoint_ >= 0 && dragAutoSlot_ == me;
    const int shown = dragging ? dragAutoPoint_ : hoverPtSlot_ == me ? hoverPt_ : -1;
    if (shown >= 0 && shown < (int) lane->points.size()) {
        const auto& pt = lane->points[(size_t) shown];
        const float px = beatToX(pt.beat);
        const float py = (float) (y + h - 3 - (pt.value - lo) / span * (h - 6));
        g.setColour(Palette::text);
        g.drawEllipse(px - 5.0f, py - 5.0f, 10.0f, 10.0f, 1.5f);
        const MeterMap meters = host().automation().meterMap();
        const auto position = timelinechrome::positionLabel(meters.barAt(pt.beat),
                                                            meters.beatInBar(pt.beat));
        timelinechrome::paintPointReadout(
            g, {kStripW, y, getWidth() - kStripW, h}, px, py,
            timelinechrome::autoPointLabel(lane->kind, pt.value, pt.valueMax,
                                           paramUnit(host(), node, slot.param), lo, hi, position));
    }
    g.restoreState();
}

void SongView::paintPodHeader(juce::Graphics& g, const trackslayout::Slot& slot) {
    const int y = slot.y, h = slot.h;
    const bool folded = collapsedPods_.count(slot.param) != 0;
    g.setColour(Palette::panelLight);
    g.fillRect(0, y, getWidth(), h - 1);
    g.setColour(Palette::border);
    g.drawHorizontalLine(y, 0.0f, (float) getWidth());

    const auto fb = podFoldBox(slot).toFloat();
    juce::Path tri;
    if (folded) tri.addTriangle(fb.getX() + 3, fb.getY() + 1, fb.getX() + 3, fb.getBottom() - 1,
                                fb.getRight() - 2, fb.getCentreY());
    else        tri.addTriangle(fb.getX() + 1, fb.getY() + 3, fb.getRight() - 1, fb.getY() + 3,
                                fb.getCentreX(), fb.getBottom() - 2);
    g.setColour(Palette::textDim);
    g.fillPath(tri);

    g.setColour(Palette::text);
    g.setFont(juce::FontOptions(11.5f));
    g.drawText(juce::String(slot.param), 18, y, kStripW - 24, h,
               juce::Justification::centredLeft, true);
    g.setColour(Palette::textDim);
    g.setFont(juce::FontOptions(9.5f));
    const int n = trackslayout::podRowCount((int) rows_.size(),
                                            [this](int t) { return podOfRow(t); }, slot.param);
    g.drawText(juce::String(n) + (n == 1 ? tr("tracks-pane-paint.lane", " lane") : tr("tracks-pane-paint.lanes", " lanes")),
               kStripW - 60, y, 54, h, juce::Justification::centredRight);

    if (folded) {
        int lo = INT_MAX, hi = 0;
        for (int t = 0; t < (int) rows_.size(); ++t) {
            if (podOfRow(t) != slot.param) continue;
            for (const auto& ci : host().clips().list(rows_[(size_t) t])) {
                lo = std::min(lo, ci.startTick);
                hi = std::max(hi, ci.startTick + ci.lengthTicks);
            }
        }
        if (hi > lo && lo != INT_MAX) {
            const float x0 = std::max((float) kStripW, tickToX(lo));
            const float x1 = tickToX(hi);
            if (x1 > x0) {
                g.setColour(Palette::accent.withAlpha(alpha::scrim));
                g.fillRoundedRectangle(x0, (float) y + 4.0f, x1 - x0, (float) h - 9.0f, 2.0f);
            }
        }
    }
}

void SongView::paintWaveform(juce::Graphics& g, juce::Rectangle<int> b, int clipLeft,
                               const ClipEditor::ClipInfo& ci, juce::Colour accent) {
    const auto* peaks = WaveformCache::instance().get(ci.audioFile, [this] { repaint(); });
    if (peaks && peaks->trouble != WaveformCache::Trouble::None) {
        g.setColour(Palette::recordRed().withAlpha(alpha::heavy));
        g.setFont(juce::FontOptions(11.0f));
        g.drawText(peaks->trouble == WaveformCache::Trouble::Missing
                       ? "file missing" : "cannot read this file",
                   b.reduced(4, 0), juce::Justification::centredLeft, true);
        return;
    }
    if (!peaks || !peaks->ready || peaks->binSamples <= 0 || peaks->sourceSamples <= 0) return;

    const double spb = (host().tempo() > 0.0 ? kSecondsPerMinute / host().tempo() : 0.5) * host().sampleRate();
    const double clipSrcLen = (double) ci.lengthTicks / Pattern::kTicksPerBeat * spb;
    const int fullW = b.getRight() - clipLeft;
    if (fullW <= 0 || clipSrcLen <= 0.0) return;

    const double sessionToFileRate = peaks->fileSampleRate > 0.0
                                         ? peaks->fileSampleRate / host().sampleRate() : 1.0;
    const float midY = b.getCentreY();
    const float halfH = b.getHeight() * 0.5f - 2.0f;
    const auto peakCol = accent.withAlpha(alpha::dim);
    const auto bodyCol = accent.brighter(0.35f).withAlpha(alpha::strong);
    const bool haveRms = peaks->rms.size() == peaks->hi.size();
    for (int x = b.getX(); x < b.getRight(); ++x) {
        auto binAt = [&](int px) {
            const double frac = (double) (px - clipLeft) / fullW;
            const double rel = ci.audioReverse ? (1.0 - frac) * clipSrcLen : frac * clipSrcLen;
            const std::int64_t sm = ci.audioOffset + (std::int64_t) rel;
            return (int) ((double) sm * sessionToFileRate / peaks->binSamples);
        };
        const int ba = binAt(x), bb = binAt(x + 1);
        const int b0 = std::min(ba, bb), b1 = std::max(b0 + 1, std::max(ba, bb));
        float hi = 0.0f, lo = 0.0f, rms = 0.0f;
        int n = 0;
        for (int k = std::max(0, b0); k < std::min(b1, (int) peaks->hi.size()); ++k) {
            hi = std::max(hi, peaks->hi[(size_t) k]);
            lo = std::min(lo, peaks->lo[(size_t) k]);
            if (haveRms) rms = std::max(rms, peaks->rms[(size_t) k]);
            ++n;
        }
        if (n == 0) continue;
        g.setColour(peakCol);
        g.drawVerticalLine(x, midY - hi * halfH, midY - lo * halfH + 1.0f);
        if (rms > 0.002f) {
            g.setColour(bodyCol);
            g.drawVerticalLine(x, midY - rms * halfH, midY + rms * halfH);
        }
    }
}

void SongView::paintNotes(juce::Graphics& g, juce::Rectangle<int> b, int clipLeft,
                            const std::string& node, const ClipEditor::ClipInfo& ci,
                            juce::Colour accent) {
    const auto notes = host().clips().notes(node, ci.index);
    if (notes.empty()) return;
    int lo = kMidiMax, hi = 0;
    for (const auto& n : notes) { lo = std::min(lo, n.pitch); hi = std::max(hi, n.pitch); }
    const auto np = timelinechrome::notePlot(b, b.getRight() - clipLeft, ci.lengthTicks,
                                             lo, hi, Pattern::kTicksPerBeat);
    if (!np.usable) return;

    g.setColour(accent.brighter(0.5f).withAlpha(np.dense ? 0.4f : 0.9f));
    for (const auto& n : notes) {
        const float x = (float) clipLeft + (float) n.tick * np.perTick;
        const float w = juce::jmax(1.0f, (float) std::max(1, n.lengthTicks) * np.perTick);
        if (x + w < (float) b.getX() || x > (float) b.getRight()) continue;
        const float x0 = juce::jmax(x, (float) b.getX());
        const float w0 = juce::jmin(x + w, (float) b.getRight()) - x0;
        if (w0 <= 0.0f) continue;
        if (np.dense) g.fillRect(x0, np.top, juce::jmin(w0, 1.5f), np.h);
        else          g.fillRect(x0, np.yFor(n.pitch), w0, np.rowH);
    }
}

}
