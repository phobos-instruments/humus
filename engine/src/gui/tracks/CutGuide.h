// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <algorithm>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/pianoroll/NoteEdit.h"
#include "gui/style/LookAndFeel.h"
#include "gui/tracks/TimelineContext.h"
#include "gui/tracks/TimelineView.h"
#include "gui/tracks/TracksGeometry.h"

namespace hum::cutguide {

inline bool active(const TimelineContext& ctx, juce::Point<int> hover) {
    return ctx.effectiveTool() == noteedit::Tool::Scissors && hover.x >= tracksgeo::kStripW
           && hover.y >= tracksgeo::headerH();
}

inline float x(const TimelineView& view, const TimelineContext& ctx, int hoverX) {
    const bool free = juce::ModifierKeys::getCurrentModifiersRealtime().isAltDown();
    return view.beatToX(ctx.snapBeats(std::max(0.0, view.xToBeat((float) hoverX, tracksgeo::kStripW)), free),
                              tracksgeo::kStripW);
}

inline void repaintMove(juce::Component& c, const TimelineView& view, const TimelineContext& ctx, int fromX,
                        int toX) {
    for (const float gx : {x(view, ctx, fromX), x(view, ctx, toX)})
        c.repaint((int) gx - 3, 0, 7, c.getHeight());
}

inline void paint(juce::Graphics& g, float x, int bottom) {
    g.setColour(Palette::text.withAlpha(alpha::strong));
    const float dash[] = {3.0f, 3.0f};
    g.drawDashedLine(juce::Line<float>(x, (float) tracksgeo::headerH(), x, (float) bottom), dash, 2,
                     1.0f);
}

}
