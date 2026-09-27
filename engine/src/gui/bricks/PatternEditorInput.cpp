// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/bricks/PatternEditor.h"
#include "gui/host/EngineHostPattern.h"

#include <algorithm>
#include <cmath>

namespace hum {

void PatternEditor::mouseMove(const juce::MouseEvent& e) {
    const bool overLane = e.x >= gridX() && laneAt(e.y) >= 0;
    setMouseCursor(overLane ? juce::MouseCursor::CrosshairCursor : juce::MouseCursor::NormalCursor);
}

void PatternEditor::mouseDown(const juce::MouseEvent& e) {
    model_.cancel();
    if (e.x < gridX()) return;
    const int lane = laneAt(e.y);
    if (lane < 0) return;
    model_.press(lane, (float) e.x, gridX(), e.mods.isCtrlDown());
    repaint();
}

void PatternEditor::mouseDrag(const juce::MouseEvent& e) {
    if (model_.drag((float) e.x, e.y, gridX(), laneTop(model_.dragLane()), kLaneH, e.mods.isCtrlDown())) repaint();
}

void PatternEditor::mouseUp(const juce::MouseEvent&) {
    model_.release();
    repaint();
}

}
