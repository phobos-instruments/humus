// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/pianoroll/RollModel.h"

#include <algorithm>
#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum::roll {

using noteedit::clampTo;

bool RollModel::pressLane(const Geometry& g, int x, int y, Mods mods, Press& result) {
    const int hit = ccPointAt(g, x, y);
    if (mods.popup) {
        if (hit >= 0 && ccSelection.count(hit) == 0) ccSelection = {hit};
        result = Press::LaneMenu;
        return true;
    }
    if (laneCC < 0) return false;
    if (mods.command) {
        if (!mods.shift) ccSelection.clear();
        gesture_ = Gesture::CCMarquee;
        dragX_ = x;
        dragY_ = y;
        marquee_ = {x, y, 0, 0};
        result = Press::Changed;
        return true;
    }
    if (hit >= 0 && tool != Tool::Draw) {
        if (!mods.shift) ccSelection = {hit};
        else if (ccSelection.count(hit) != 0) ccSelection.erase(hit);
        else ccSelection.insert(hit);
        result = Press::Changed;
        return true;
    }
    ccSelection.clear();
    return false;
}

RollModel::Press RollModel::press(const Geometry& g, int x, int y, Mods mods, int& hit) {
    nudgeOpen_ = false;
    hit = -1;
    if (g.inLane(x, y)) {
        Press result = Press::Nothing;
        if (pressLane(g, x, y, mods, result)) return result;
        clip_.pushUndo();
        if (laneCC < 0) {
            gestureNotes_ = notes();
            gesture_ = Gesture::VelLane;
            applyVelocityLane(g, x, y);
        } else {
            gestureCCs_ = clip_.ccs();
            gesture_ = Gesture::CCLane;
            applyCCLane(g, x, y);
        }
        return Press::Changed;
    }
    if (g.inKeys(x, y) && !mods.popup) {
        gesture_ = Gesture::Keys;
        keyPitch_ = clampTo(g.pitchAt(y), 0, kMidiMax);
        return Press::Keys;
    }
    if (!g.inGrid(x, y)) return Press::Nothing;
    const int pitch = g.pitchAt(y);
    if (pitch < 0 || pitch > kMidiMax) return Press::Nothing;
    const int tick = g.clampTick(g.xToTick((float) x));
    ccSelection.clear();

    noteedit::Grab grab = noteedit::Grab::Miss;
    hit = noteAt(g, tick, pitch, grab, x);

    if (mods.popup) {
        if (hit >= 0 && selection.count(hit) == 0) selection = {hit};
        return selection.empty() ? Press::Changed : Press::NoteMenu;
    }

    if (tool == Tool::Scissors) {
        if (hit >= 0) split(g, hit, tick);
        return Press::Changed;
    }

    if (tool == Tool::Eraser) {
        clip_.pushUndo();
        gestureNotes_ = notes();
        selection.clear();
        gesture_ = Gesture::Erase;
        if (hit >= 0) gestureNotes_.erase(gestureNotes_.begin() + hit);
        return Press::Changed;
    }

    dragX_ = x;
    dragY_ = y;

    if (tool == Tool::Pointer) {
        if (hit < 0) {
            if (!mods.shift) selection.clear();
            gesture_ = Gesture::Marquee;
            marquee_ = {x, y, 0, 0};
            return Press::Changed;
        }
        if (mods.shift) {
            if (selection.count(hit)) selection.erase(hit);
            else selection.insert(hit);
            return Press::Changed;
        }
        if (!selection.count(hit)) {
            selection.clear();
            selection.insert(hit);
        }
        if (selection.size() > 1 && grab == noteedit::Grab::Body && !mods.alt) {
            clip_.pushUndo();
            gestureBase_ = notes();
            gestureNotes_ = gestureBase_;
            gesture_ = Gesture::MoveGroup;
            gestureStartTick_ = tick;
            gestureStartPitch_ = pitch;
            gestureIndex_ = -1;
            return Press::Changed;
        }
    }

    clip_.pushUndo();
    gestureNotes_ = notes();

    if (hit >= 0 && mods.alt) {
        gesture_ = Gesture::Velocity;
        gestureIndex_ = hit;
        gestureStartVel_ = gestureNotes_[(size_t) hit].velocity;
    } else if (hit >= 0 && grab == noteedit::Grab::RightEdge) {
        gesture_ = Gesture::Resize;
        gestureIndex_ = hit;
    } else if (hit >= 0 && grab == noteedit::Grab::LeftEdge) {
        gesture_ = Gesture::ResizeL;
        gestureIndex_ = hit;
    } else if (hit >= 0) {
        gesture_ = Gesture::Move;
        gestureIndex_ = hit;
        selection.clear();
        selection.insert(hit);
        gestureStartTick_ = gestureNotes_[(size_t) hit].tick;
        gestureStartPitch_ = gestureNotes_[(size_t) hit].pitch;
        gestureTickOffset_ = tick - gestureNotes_[(size_t) hit].tick;
    } else if (tool == Tool::Draw) {
        gesture_ = Gesture::Create;
        NoteEvent n;
        n.tick = g.snapTick(tick);
        n.pitch = pitch;
        n.lengthTicks = g.snap;
        n.velocity = 100;
        selection.clear();
        gestureNotes_.push_back(n);
        gestureIndex_ = (int) gestureNotes_.size() - 1;
    } else {
        gesture_ = Gesture::None;
        return Press::Nothing;
    }
    return Press::Changed;
}

bool RollModel::drag(const Geometry& g, int x, int y, Mods mods) {
    if (gesture_ == Gesture::Keys) {
        const int pitch = clampTo(g.pitchAt(y), 0, kMidiMax);
        if (pitch == keyPitch_) return false;
        keyPitch_ = pitch;
        return true;
    }
    const int tick = g.xToTick((float) x);
    switch (gesture_) {
        case Gesture::VelLane: applyVelocityLane(g, x, y); return true;
        case Gesture::CCLane: applyCCLane(g, x, y); return true;
        case Gesture::CCMarquee:
            marquee_ = Rect::between(dragX_, dragY_, x, y);
            ccMarqueeSelect(g, marquee_, mods.shift);
            return true;
        case Gesture::Marquee:
            marquee_ = Rect::between(dragX_, dragY_, x, y);
            marqueeSelect(g, marquee_, mods.shift);
            return true;
        case Gesture::Erase: {
            const int pitch = g.pitchAt(y);
            for (int i = (int) gestureNotes_.size() - 1; i >= 0; --i) {
                const auto& n = gestureNotes_[(size_t) i];
                if (n.pitch == pitch && tick >= n.tick && tick < n.tick + n.lengthTicks) {
                    gestureNotes_.erase(gestureNotes_.begin() + i);
                    return true;
                }
            }
            return false;
        }
        case Gesture::MoveGroup: {
            const int dTick = noteedit::snapDelta(tick - gestureStartTick_, g.step());
            const int dPitch = clampTo(g.pitchAt(y), 0, kMidiMax) - gestureStartPitch_;
            gestureNotes_ = gestureBase_;
            for (int i : selection) {
                if (i < 0 || i >= (int) gestureNotes_.size()) continue;
                auto& n = gestureNotes_[(size_t) i];
                n.tick = clampTo(n.tick + dTick, 0, g.duration - 1);
                n.pitch = clampTo(n.pitch + dPitch, 0, kMidiMax);
            }
            return true;
        }
        default:
            return dragNotes(g, x, y, tick);
    }
}

bool RollModel::dragNotes(const Geometry& g, int, int y, int tick) {
    if (gesture_ == Gesture::None || gestureIndex_ < 0 || gestureIndex_ >= (int) gestureNotes_.size()) return false;
    auto& n = gestureNotes_[(size_t) gestureIndex_];
    switch (gesture_) {
        case Gesture::Create:
        case Gesture::Resize: {
            const int end = clampTo(tick, n.tick + 1, g.duration);
            const int snapped = g.free ? end : g.snapTick(end - 1) + g.snap;
            n.lengthTicks = std::max(g.free ? 1 : g.snap / 2, snapped - n.tick);
            break;
        }
        case Gesture::ResizeL: {
            const int head = g.snapTick(clampTo(tick, 0, g.duration - 1));
            const auto r = noteedit::resizeLeft(n.tick, n.lengthTicks, head, g.free ? 1 : std::max(1, g.snap / 2));
            n.tick = r.tick;
            n.lengthTicks = r.lengthTicks;
            break;
        }
        case Gesture::Move: {
            const int moved = noteedit::snapDelta((tick - gestureTickOffset_) - gestureStartTick_, g.step());
            n.tick = clampTo(gestureStartTick_ + moved, 0, g.duration - 1);
            n.pitch = clampTo(g.pitchAt(y), 0, kMidiMax);
            break;
        }
        case Gesture::Velocity:
            n.velocity = clampTo(gestureStartVel_ + (dragY_ - y), 1, kMidiMax);
            break;
        default:
            break;
    }
    return true;
}

bool RollModel::release(const Geometry& g) {
    if (gesture_ == Gesture::Keys) {
        gesture_ = Gesture::None;
        keyPitch_ = -1;
        return false;
    }
    if (gesture_ == Gesture::None) return false;
    if (gesture_ == Gesture::Marquee || gesture_ == Gesture::CCMarquee) {
        gesture_ = Gesture::None;
        marquee_ = {};
        return true;
    }
    if (gesture_ == Gesture::CCLane) {
        clip_.setCCs(gestureCCs_);
        gesture_ = Gesture::None;
        gestureCCs_.clear();
        return true;
    }
    commit(g, gestureNotes_);
    gesture_ = Gesture::None;
    gestureIndex_ = -1;
    gestureNotes_.clear();
    gestureBase_.clear();
    return true;
}

bool RollModel::doubleClick(const Geometry& g, int x, int y) {
    if (!g.inGrid(x, y)) return false;
    noteedit::Grab grab = noteedit::Grab::Miss;
    const int pitch = g.pitchAt(y);
    const int tick = g.clampTick(g.xToTick((float) x));
    const int hit = noteAt(g, tick, pitch, grab, x);
    if (hit >= 0) {
        clip_.pushUndo();
        auto n = notes();
        n.erase(n.begin() + hit);
        selection.clear();
        commit(g, n);
        return true;
    }
    if (tool != Tool::Pointer) return false;
    clip_.pushUndo();
    auto n = notes();
    NoteEvent ne;
    ne.tick = g.snapTick(tick);
    ne.pitch = pitch;
    ne.lengthTicks = g.snap;
    ne.velocity = 100;
    selection.clear();
    selection.insert((int) n.size());
    n.push_back(ne);
    commit(g, n);
    return true;
}

void RollModel::applyVelocityLane(const Geometry& g, int x, int y) {
    const int vel = clampTo((int) std::nearbyint(kMidiMaxD * ((g.gridBottom + g.velH - 3) - y) / (double) (g.velH - 6)), 1,
                            kMidiMax);
    for (size_t i = 0; i < gestureNotes_.size(); ++i) {
        if (!selection.empty()) {
            if (!selection.count((int) i)) continue;
        } else if (std::abs((float) x - g.tickToX(gestureNotes_[i].tick)) > 4.0f) {
            continue;
        }
        gestureNotes_[i].velocity = vel;
    }
}

void RollModel::applyCCLane(const Geometry& g, int x, int y) {
    const int top = ccValueMax(laneCC);
    const int value =
        clampTo((int) std::nearbyint((double) top * ((g.gridBottom + g.velH - 3) - y) / (double) (g.velH - 6)), 0, top);
    const int tick = g.snapTick(g.clampTick(g.xToTick((float) x)));
    bool placed = false;
    for (auto& c : gestureCCs_)
        if (c.controller == laneCC && c.tick == tick) {
            c.value = value;
            placed = true;
        }
    if (!placed) gestureCCs_.push_back({tick, laneCC, value});
}

}
