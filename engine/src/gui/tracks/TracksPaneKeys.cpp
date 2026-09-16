// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/tracks/TracksPane.h"

#include "gui/app/QwertyPiano.h"

namespace hum {

TracksPane::Tool TracksPane::effectiveTool() const {
    if (!QwertyPiano::instance().enabled()) {
        if (juce::KeyPress::isKeyCurrentlyDown('x')) return Tool::Scissors;
        if (juce::KeyPress::isKeyCurrentlyDown('e')) return Tool::Eraser;
    }
    return tool_;
}

void TracksPane::selectTool(Tool tool) {
    const bool guide = tool == Tool::Scissors || tool_ == Tool::Scissors;
    tool_ = tool;
    ruler_.repaint(0, tracksgeo::rulerTop(), kStripW, tracksgeo::kRulerH);
    if (!guide) return;
    if (mode_ == Mode::Clip) clipView_.repaintCutGuide();
    else if (mode_ == Mode::Track) rollView_.repaintCutGuide();
    else song_.repaintCutGuide();
}

bool TracksPane::keyPressed(const juce::KeyPress& k) {
    if (mode_ == Mode::Clip) return clipView_.keyPressed(k);
    if (mode_ == Mode::Track && rollView_.keyPressed(k)) return true;
    for (int i = 0; i < tracksgeo::kToolCount; ++i)
        if (k == juce::KeyPress('1' + i)) {
            selectTool(TimelineRuler::toolAt(i));
            return true;
        }
    return mode_ == Mode::Song && song_.keyPressed(k);
}

}
