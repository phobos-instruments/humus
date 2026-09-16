// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/tracks/TrackRollView.h"
#include "gui/host/TracksHost.h"
#include <algorithm>
#include <climits>
#include <cmath>
#include <iterator>
#include "core/timeline/ClipOps.h"
#include "gui/common/Localisation.h"
#include "gui/host/EngineHostAutomation.h"
#include "gui/pianoroll/NoteEdit.h"
#include "gui/style/LookAndFeel.h"
#include "gui/tracks/ClipColors.h"
#include "gui/tracks/CutGuide.h"
#include "hum/ClipStack.h"
#include "io/PatchDocument.h"
#include "hum/dsp/DspMath.h"
#include "gui/tracks/NotePlot.h"
#include "gui/tracks/TimelineChips.h"
#include "gui/tracks/TimelineGrid.h"

namespace hum {

void TrackRollView::paint(juce::Graphics& g) {
    g.setOrigin(-getPosition());
    paintRoll(g);
    if (cutguide::active(ctx_, hover_)) cutguide::paint(g, cutguide::x(view_, ctx_, hover_.x), getBottom());
    paintRollSelection(g);
    timelinechrome::paintPlayhead(g, view_.beatToX(view_.playBeat, tracksgeo::kStripW), (float) tracksgeo::rulerTop(),
                                  (float) getBottom(), (float) tracksgeo::kStripW, (float) getWidth(), true);
}

void TrackRollView::paintRoll(juce::Graphics& g) {
    const auto f = rollField();
    const auto rp = rollPlot();
    const auto accent = timelinechrome::laneAccent(host().model(), node_);

    g.setColour(Palette::background);
    g.fillRect(0, tracksgeo::headerH(), getWidth(), getBottom() - tracksgeo::headerH());
    g.setColour(accent.withAlpha(alpha::wash));
    g.fillRect(f);
    g.setColour(Palette::panel);
    g.fillRect(0, tracksgeo::headerH(), getWidth(), tracksgeo::kChipH);
    g.setColour(Palette::border.withAlpha(alpha::dim));
    g.drawHorizontalLine(tracksgeo::headerH() + tracksgeo::kChipH - 1, 0.0f, (float) getWidth());
    g.setColour(accent);
    g.fillRect(6, tracksgeo::headerH() + 7, 8, 8);
    g.setColour(Palette::text);
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText(juce::String(node_), 20, tracksgeo::headerH(), tracksgeo::kStripW - 80, tracksgeo::kChipH,
               juce::Justification::centredLeft, true);
    timelinechrome::paintDestChip(g, {tracksgeo::kStripW + 8, tracksgeo::headerH() + 4, 140, tracksgeo::kChipH - 8},
                                  host().model(), node_);

    if (!rp.usable) return;

    g.saveState();
    g.reduceClipRegion(f);
    if (rp.rowH >= 4.0f) {
        for (int i = 0; i <= rp.rows(); ++i) {
            const int pitch = rp.topPitch - i;
            if (pitch < 0 || pitch > kMidiMax) continue;
            const float y = rp.yFor(pitch);
            g.setColour(timelinechrome::isBlackKey(pitch) ? Palette::background.brighter(0.02f)
                                                          : Palette::background.brighter(0.055f));
            g.fillRect((float) f.getX(), y, (float) f.getWidth(), rp.hFor(pitch));
            g.setColour(Palette::border.withAlpha(noteedit::laneRuleAlpha(pitch)));
            g.drawHorizontalLine((int) (y + rp.hFor(pitch) - 1.0f), (float) f.getX(),
                                 (float) f.getRight());
        }
    }

    timelinechrome::paintTimeGrid(g, f, tracksgeo::kStripW, view_.scrollBeats, view_.ppb,
                                  host().automation().meterMap(), view_.snapChoice);

    const auto* cm = host().model().byName(node_);
    const auto clips = host().clips().list(node_);
    const int ribbonY = ribbonTop();
    for (const auto& ci : clips) {
        const auto col = ci.color > 0 ? clipColour(ci.color) : accent;
        std::vector<clipstack::Span> spans;
        if (cm != nullptr) {
            const auto chans = clipops::clipChannels(cm->pattern);
            if (ci.index < (int) chans.size())
                spans = clipstack::soundingSpans(cm->pattern, chans[(size_t) ci.index]);
        }
        if (spans.empty()) spans.push_back({ci.startTick, ci.lengthTicks});
        for (const auto& sp : spans) {
            const float x0 = std::max(tickToX(sp.startTick), (float) tracksgeo::kStripW);
            const float x1 = std::min(tickToX(sp.startTick + sp.lengthTicks),
                                      (float) getWidth());
            if (x1 <= x0) continue;
            g.setColour(col.withAlpha(alpha::wash));
            g.fillRect(x0, (float) f.getY(), x1 - x0, (float) f.getHeight());
            g.setColour(col.withAlpha(sp.startTick == ci.startTick ? 0.85f : 0.35f));
            g.fillRect(x0, (float) ribbonY, x1 - x0, (float) kRibbonH);
        }
        const float sx = tickToX(ci.startTick);
        if (sx >= (float) tracksgeo::kStripW && sx <= (float) getWidth()) {
            g.setColour(col.withAlpha(alpha::scrim));
            g.drawVerticalLine((int) sx, (float) f.getY(), (float) f.getBottom());
        }
        if (const float w = tickToX(ci.startTick + ci.lengthTicks) - sx;
            w > 40.0f && sx < (float) getWidth()) {
            g.setColour(Palette::textDim);
            g.setFont(juce::FontOptions(9.5f));
            g.drawText(ci.name.empty() ? tr("tracks-pane-roll.clip", "clip ") + juce::String(ci.index + 1)
                                       : juce::String(ci.name),
                       (int) std::max(sx + 9.0f, (float) tracksgeo::kStripW + 3.0f),
                       ribbonY + kRibbonH + 1, 120, 11, juce::Justification::centredLeft, false);
        }
    }

    paintClipHandles(g);

    const bool showVel = rp.rowH >= 3.0f;
    for (const auto& ci : clips) {
        const auto col = ci.color > 0 ? clipColour(ci.color) : accent;
        for (const auto& n : host().clips().notes(node_, ci.index)) {
            const int at = ci.startTick + n.tick;
            const float x = tickToX(at);
            const float w = std::max(2.0f, tickToX(at + std::max(1, n.lengthTicks)) - x);
            if (x + w < (float) tracksgeo::kStripW || x > (float) getWidth()) continue;
            const float y = rp.yFor(n.pitch);
            const float nh = rp.hFor(n.pitch);
            if (y + nh < (float) f.getY() || y > (float) f.getBottom()) continue;
            const float x0 = std::max(x, (float) tracksgeo::kStripW);
            const float w0 = std::min(x + w, (float) getWidth()) - x0;
            g.setColour(showVel ? col.withBrightness(juce::jlimit(
                                      0.25f, 1.0f, 0.45f + 0.55f * n.velocity / kMidiMaxF))
                                : col.withAlpha(alpha::heavy));
            if (rp.rowH >= 5.0f)
                g.fillRoundedRectangle(x0, y, w0, std::max(2.0f, nh - 1.0f), 2.0f);
            else
                g.fillRect(x0, y, w0, std::max(2.0f, nh - 1.0f));
            noteedit::paintNoteName(g, {x0, y, w0, nh - 1.0f}, n.pitch,
                                    Palette::background.withAlpha(alpha::strong));
            if (rp.rowH >= 7.0f && w0 >= 6.0f) {
                g.setColour(col.darker(0.5f));
                g.drawRoundedRectangle(x0, y, w0, nh - 1.0f, 2.0f, 1.0f);
            }
        }
    }
    g.restoreState();

    g.setColour(Palette::panel);
    g.fillRect(tracksgeo::kStripW - kKeyW, f.getY(), kKeyW, f.getHeight());
    std::set<int> lit;
    if (keyNote_ >= 0) lit.insert(keyNote_);
    {
        const int at = (int) std::llround(view_.playBeat * Pattern::kTicksPerBeat);
        for (const auto& ci : host().clips().list(node_))
            for (const auto& n : host().clips().notes(node_, ci.index)) {
                const int t0 = ci.startTick + n.tick;
                if (at >= t0 && at < t0 + std::max(1, n.lengthTicks)) lit.insert(n.pitch);
            }
    }
    g.saveState();
    g.reduceClipRegion(tracksgeo::kStripW - kKeyW, f.getY(), kKeyW, f.getHeight());
    if (rp.rowH >= 2.5f)
        for (int i = 0; i <= rp.rows(); ++i) {
            const int pitch = rp.topPitch - i;
            if (pitch < 0 || pitch > kMidiMax) continue;
            const float y = rp.yFor(pitch);
            const float kh = rp.hFor(pitch);
            if (y + kh < (float) f.getY() || y > (float) f.getBottom()) continue;
            noteedit::paintPianoKey(g, {(float) (tracksgeo::kStripW - kKeyW), y, (float) kKeyW - 1.0f,
                                        std::max(1.0f, kh)},
                                    pitch, lit.count(pitch) != 0, Palette::accent);
            if (((pitch % 12) + 12) % 12 == 0 && rp.rowH >= 8.0f) {
                g.setColour(juce::Colour(noteedit::kEbony));
                g.setFont(juce::FontOptions(9.0f));
                g.drawText("C" + juce::String(pitch / 12 - 1), tracksgeo::kStripW - kKeyW + 2,
                           (int) y - 1, kKeyW - 5, (int) kh + 2,
                           juce::Justification::centredRight, false);
            }
        }
    g.restoreState();
    g.setColour(Palette::border);
    g.drawVerticalLine(tracksgeo::kStripW - 1, (float) f.getY(), (float) f.getBottom());

    if (rollShowsVelocity()) {
        const int top = getBottom() - velH_;
        g.setColour(Palette::background.darker(0.15f));
        g.fillRect(0, top, getWidth(), velH_);
        g.setColour(Palette::border);
        g.drawHorizontalLine(top, 0.0f, (float) getWidth());
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(9.0f));
        g.drawText("vel", 4, top + 3, kKeyW - 8, 10, juce::Justification::centredLeft, false);
        for (const auto& ci : clips) {
            const auto col = ci.color > 0 ? clipColour(ci.color) : accent;
            g.setColour(col.withAlpha(alpha::dim));
            for (const auto& n : host().clips().notes(node_, ci.index)) {
                const float x = tickToX(ci.startTick + n.tick);
                if (x < (float) tracksgeo::kStripW || x > (float) getWidth()) continue;
                const float h = ((float) velH_ - 6.0f) * n.velocity / kMidiMaxF;
                const float barTop = (float) (top + velH_ - 3) - h;
                g.setColour(col.withAlpha(alpha::dim));
                g.fillRect(x, barTop, 3.0f, h);
                g.setColour(col);
                g.fillEllipse(x - 1.5f, barTop - 3.0f, 6.0f, 6.0f);
            }
        }
        if (velShowX_ >= 0 && velShowVal_ >= 0) {
            const auto bubble = juce::Rectangle<int>(
                juce::jlimit(tracksgeo::kStripW, getWidth() - 40, velShowX_ - 17), top - 18, 34, 16);
            g.setColour(Palette::accent);
            g.fillRoundedRectangle(bubble.toFloat(), 3.0f);
            g.setColour(Palette::background);
            g.setFont(juce::FontOptions(10.5f));
            g.drawText(juce::String(velShowVal_), bubble, juce::Justification::centred);
        }
    }

    if (clips.empty()) {
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(14.0f));
        auto r = f.reduced(0, f.getHeight() / 3);
        g.drawText(tr("tracks-pane-roll.no-clips-on-this-track", "No clips on this track yet"), r.removeFromTop(r.getHeight() / 2),
                   juce::Justification::centredBottom);
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(12.0f));
        g.drawText(tr("tracks-pane-roll.drag-one-in-or-arm", "Drag one in, or arm R and play"), r, juce::Justification::centredTop);
    }
}

void TrackRollView::paintClipHandles(juce::Graphics& g) {
    const auto clips = host().clips().list(node_);
    if (selClip_ < 0 || selClip_ >= (int) clips.size()) return;
    const auto& ci = clips[(size_t) selClip_];
    const auto f = rollField();
    const float x0 = std::max(tickToX(ci.startTick), (float) tracksgeo::kStripW);
    const float x1 = std::min(tickToX(ci.startTick + ci.lengthTicks), (float) getWidth());
    g.setColour(Palette::accent.withAlpha(alpha::strong));
    if (x1 > x0)
        g.drawRect(juce::Rectangle<float>(x0, (float) ribbonTop(), x1 - x0,
                                          (float) f.getHeight() + kRibbonH), 1.5f);
    g.setColour(Palette::accent);
    for (const bool left : {true, false}) {
        const auto h = clipHandle(left);
        if (h.getRight() >= tracksgeo::kStripW && h.getX() <= getWidth())
            g.fillRoundedRectangle(h.toFloat(), 3.0f);
    }
}

void TrackRollView::paintRollSelection(juce::Graphics& g) {
    if (node_.empty()) return;
    if (drag_ == Drag::Marquee && !marquee_.isEmpty()) {
        g.setColour(Palette::accent.withAlpha(alpha::mist));
        g.fillRect(marquee_);
        g.setColour(Palette::accent);
        g.drawRect(marquee_, 1);
    }
    if (sel_.empty()) return;
    const auto f = rollField();
    g.saveState();
    g.reduceClipRegion(f);
    g.setColour(Palette::text);
    for (const auto& ci : host().clips().list(node_)) {
        const auto notes = host().clips().notes(node_, ci.index);
        for (int i = 0; i < (int) notes.size(); ++i) {
            if (sel_.count({ci.index, i}) == 0) continue;
            const auto b = noteBounds(ci.startTick, notes[(size_t) i]);
            if (b.getRight() < (float) tracksgeo::kStripW || b.getX() > (float) getWidth()) continue;
            g.drawRoundedRectangle(b.expanded(1.0f, 1.0f), 2.0f, 1.2f);
        }
    }
    g.restoreState();
}

}
