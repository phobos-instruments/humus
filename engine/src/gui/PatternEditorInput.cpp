#include "gui/PatternEditor.h"

#include <algorithm>
#include <cmath>

namespace hum {

void PatternEditor::mouseMove(const juce::MouseEvent& e) {
    const bool overLane = e.x >= gridX() && laneAt(e.y) >= 0;
    setMouseCursor(overLane ? juce::MouseCursor::CrosshairCursor : juce::MouseCursor::NormalCursor);
}

void PatternEditor::mouseDown(const juce::MouseEvent& e) {
    dragLane_ = -1; dragAdded_ = dragMoved_ = deleteOnUp_ = false;
    if (e.x < gridX()) return;
    const int lane = laneAt(e.y);
    if (lane < 0) return;

    const int rawTick = std::max(0, xToTick((float) e.x));
    const int tolTicks = std::max(1, (int) std::lround(7.0 / ppt()));
    const int hit = nearestTrigger(lane, rawTick, tolTicks);

    host_.pushUndo();
    dragLane_ = lane;
    if (hit >= 0) {
        dragTick_ = hit;
    } else {
        const int t = e.mods.isCtrlDown() ? rawTick : snapTickForLane(lane, rawTick);
        host_.patterns().addTrigger(name_, lane, t);
        dragTick_ = t; dragAdded_ = true;
    }
    repaint();
}

void PatternEditor::mouseDrag(const juce::MouseEvent& e) {
    if (dragLane_ < 0) return;
    const int top = laneTop(dragLane_);
    deleteOnUp_ = e.y < top - 10 || e.y > top + kLaneH + 10;

    const int rawTick = std::max(0, xToTick((float) e.x));
    const int newTick = e.mods.isCtrlDown() ? rawTick : snapTickForLane(dragLane_, rawTick);
    if (!deleteOnUp_ && newTick != dragTick_) {
        host_.patterns().moveTrigger(name_, dragLane_, dragTick_, newTick);
        dragTick_ = newTick; dragMoved_ = true;
    }
    repaint();
}

void PatternEditor::mouseUp(const juce::MouseEvent&) {
    if (dragLane_ >= 0) {
        if (deleteOnUp_)
            host_.patterns().removeTrigger(name_, dragLane_, dragTick_);
        else if (!dragAdded_ && !dragMoved_)
            host_.patterns().removeTrigger(name_, dragLane_, dragTick_);
    }
    dragLane_ = -1; dragAdded_ = dragMoved_ = deleteOnUp_ = false;
    repaint();
}

}
