// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/AutomateMenu.h"
#include "gui/style/Colours.h"
#include "gui/tracks/SongView.h"
#include <climits>
#include <cmath>
#include <cstdint>
#include "gui/style/EnvelopePainter.h"
#include "gui/style/LookAndFeel.h"
#include "gui/tracks/TimelineGrid.h"
#include "gui/tracks/WaveformCache.h"
#include "gui/common/Localisation.h"

namespace hum {

void SongView::paintBoxes(juce::Graphics& g, int row) {
    const auto& node = rows_[(size_t) row];
    const auto cb = g.getClipBounds();
    const auto& boxes = host().automation().boxes();
    const auto* cm = host().model().byName(node);
    for (int i = 0; i < (int) boxes.size(); ++i) {
        if (boxes[(size_t) i].organism != node) continue;
        auto b = boxBounds(row, boxes[(size_t) i]);
        if (b.getRight() < kStripW || b.getX() > getWidth()) continue;
        b.setLeft(std::max(b.getX(), kStripW));
        if (b.getRight() < cb.getX() || b.getX() > cb.getRight()) continue;
        const bool sel = i == selBox_ || boxSelected(i);
        g.setColour((sel ? Palette::accent : Palette::accentDim).withAlpha(alpha::scrim));
        g.fillRoundedRectangle(b.toFloat(), 3.0f);
        g.setColour(sel ? Palette::accent : Palette::accentDim);
        g.drawRoundedRectangle(b.toFloat(), 3.0f, sel ? 1.6f : 1.1f);
        if (b.getWidth() > 24) {
            const int gy = b.getCentreY() - 4;
            g.fillRoundedRectangle((float) b.getX() + 2.0f, (float) gy, 2.0f, 8.0f, 1.0f);
            g.fillRoundedRectangle((float) b.getRight() - 4.0f, (float) gy, 2.0f, 8.0f, 1.0f);
        }
        if (cm != nullptr && b.getWidth() > 14) {
            const auto& box = boxes[(size_t) i];
            const double delta = dragBox_ == i ? boxDragDelta_ : 0.0;
            g.saveState();
            g.reduceClipRegion(b);
            int drawn = 0;
            for (const auto& l : cm->automation) {
                if (drawn >= 3 || l.points.empty()) continue;
                double lo = 1e18, hi = -1e18;
                for (const auto& pt : l.points) {
                    lo = std::min({lo, pt.value, pt.valueMax});
                    hi = std::max({hi, pt.value, pt.valueMax});
                }
                if (hi <= lo) { lo -= 0.5; hi += 0.5; }
                juce::Path path;
                bool first = true;
                for (const auto& pt : l.points) {
                    if (pt.beat < box.startBeat || pt.beat > box.endBeat) continue;
                    const float px = beatToX(pt.beat + delta);
                    const float py = (float) (b.getBottom() - 3
                        - (pt.value - lo) / (hi - lo) * (b.getHeight() - 8));
                    if (first) { path.startNewSubPath(px, py); first = false; }
                    else path.lineTo(px, py);
                }
                g.setColour(Palette::text.withAlpha(0.45f - 0.1f * (float) drawn));
                g.strokePath(path, juce::PathStrokeType(1.0f));
                ++drawn;
            }
            g.restoreState();
            g.setColour(Palette::text.withAlpha(alpha::heavy));
            g.setFont(juce::FontOptions(9.0f));
            int nLanes = 0;
            for (const auto& l : cm->automation) if (!l.points.empty()) ++nLanes;
            if (b.getWidth() > 40)
                g.drawText(juce::String(nLanes) + (nLanes == 1 ? tr("tracks-pane-paint.param", " param") : tr("tracks-pane-paint.params", " params")),
                           b.reduced(4, 1), juce::Justification::topLeft, false);
        }
    }
}

void SongView::paintBoxRow(juce::Graphics& g, const trackslayout::Slot& s) {
    const auto& node = rows_[(size_t) s.track];
    g.setColour(Palette::panel);
    g.fillRect(0, s.y, kStripW, s.h - 1);
    const auto fb = boxFoldBox(s).toFloat();
    const bool open = expanded_.count(node) != 0;
    juce::Path tri;
    if (open) tri.addTriangle(fb.getX() + 2, fb.getY() + 4, fb.getRight() - 2, fb.getY() + 4,
                              fb.getCentreX(), fb.getBottom() - 3);
    else      tri.addTriangle(fb.getX() + 4, fb.getY() + 2, fb.getX() + 4, fb.getBottom() - 2,
                              fb.getRight() - 3, fb.getCentreY());
    g.setColour(Palette::textDim);
    g.fillPath(tri);
    g.setFont(juce::FontOptions(11.0f));
    g.drawText(tr("tracks-pane-paint.automation", "Automation"), 20, s.y, kStripW - 24, s.h - 1, juce::Justification::centredLeft, false);
    g.setColour(Palette::background.brighter(0.035f));
    g.fillRect(kStripW, s.y, getWidth() - kStripW, s.h - 1);
    timelinechrome::paintTimeGrid(g, {kStripW, s.y, getWidth() - kStripW, s.h - 1}, kStripW,
                                  view_.scrollBeats, view_.ppb, host().automation().meterMap(), view_.snapChoice);
    paintBoxes(g, s.track);
}

void SongView::paintLinePreview(juce::Graphics& g) {
    if (drag_ != Drag::Line || lineSlot_ < 0 || lineSlot_ >= (int) slots_.size()) return;
    const auto& sl = slots_[(size_t) lineSlot_];
    const auto [lo, hi] = laneRange(dragAutoNode_, dragAutoParam_);
    g.setColour(Palette::accent.withAlpha(alpha::nearOpaque));
    g.drawLine(beatToX(lineBeat0_), laneYAtValue(sl, lineVal0_, lo, hi),
               beatToX(lineBeat1_), laneYAtValue(sl, lineVal1_, lo, hi), 1.5f);
}

}
