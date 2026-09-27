// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <set>
#include <vector>

#include "gui/editor/Geometry.h"
#include "gui/pianoroll/NoteRules.h"
#include "hum/PatternMatrix.h"
#include "hum/Swing.h"

namespace hum::roll {

class ClipNotes {
public:
    virtual ~ClipNotes() = default;
    virtual std::vector<NoteEvent> notes() const = 0;
    virtual void setNotes(const std::vector<NoteEvent>& notes, int durationTicks) = 0;
    virtual std::vector<CCEvent> ccs() const = 0;
    virtual void setCCs(const std::vector<CCEvent>& ccs) = 0;
    virtual void pushUndo() = 0;
};

struct ClipSpan {
    int startTick = 0, lengthTicks = 0;
    bool looped = false;
};

struct Playhead {
    double tick = -1.0;
    bool preview = false;
    bool playing = false;
};

double clipTick(const std::vector<ClipSpan>& clips, int clip, double beats);
Playhead playheadAt(const std::vector<ClipSpan>& clips, int clip, double beats, int durationTicks, bool playing);
int durationOf(const std::vector<ClipSpan>& clips, int clip, int patternTicks);

enum class LoopTap { CloseTake, Disarm, StartTake, Overdub };
LoopTap loopTapFor(bool armed, bool takeOpen, bool hasNotes);

struct Geometry {
    int gridLeft = 0, gridTop = 0, gridBottom = 0, rowH = 13, velH = 42;
    int topPitch = 83;
    double ppt = 1.0;
    int duration = 1;
    int snap = 1;
    bool free = false;
    int barTicks = 1;

    float tickToX(double tick) const { return (float) (gridLeft + tick * ppt); }
    int xToTick(float x) const { return (int) std::lround((x - gridLeft) / ppt); }
    int pitchAt(int y) const { return topPitch - (y - gridTop) / rowH; }
    float pitchToY(int pitch) const { return (float) (gridTop + (topPitch - pitch) * rowH); }
    int snapTick(int tick) const;
    int step() const { return free ? 1 : snap; }
    int clampTick(int tick) const;
    int rows() const;
    RectF noteBox(const NoteEvent& e) const;
    float ccY(int value, int laneCC) const;
    bool inLane(int x, int y) const { return y >= gridBottom && y < gridBottom + velH && x >= gridLeft; }
    bool inKeys(int x, int y) const { return x < gridLeft && y >= gridTop && y < gridBottom; }
    bool inGrid(int x, int y) const { return y >= gridTop && y < gridBottom && x >= gridLeft; }
};

struct Mods {
    bool shift = false, alt = false, command = false, ctrl = false, popup = false;
};

class RollModel {
public:
    using Tool = noteedit::Tool;

    enum class Gesture { None, Create, Move, MoveGroup, Resize, ResizeL, Velocity, Marquee, Erase, VelLane, CCLane,
                         CCMarquee, Keys };

    enum class Press { Nothing, Changed, NoteMenu, LaneMenu, Keys };

    explicit RollModel(ClipNotes& clip) : clip_(clip) {}

    Tool tool = Tool::Pointer;
    std::set<int> selection;
    std::set<int> ccSelection;
    int laneCC = -1;
    int topPitch = 83;
    bool pitchScrolled = false;

    std::vector<NoteEvent> notes() const { return clip_.notes(); }
    std::vector<CCEvent> ccs() const { return clip_.ccs(); }
    Gesture gesture() const { return gesture_; }
    bool editsNotes() const {
        return gesture_ != Gesture::None && gesture_ != Gesture::Marquee && gesture_ != Gesture::CCLane
               && gesture_ != Gesture::CCMarquee;
    }
    const std::vector<NoteEvent>& gestureNotes() const { return gestureNotes_; }
    const std::vector<CCEvent>& gestureCCs() const { return gestureCCs_; }
    const Rect& marquee() const { return marquee_; }
    int keyPitch() const { return keyPitch_; }
    int gestureIndex() const { return gestureIndex_; }

    int noteAt(const Geometry& g, int tick, int pitch, noteedit::Grab& grab, int x) const;
    int ccPointAt(const Geometry& g, int x, int y) const;

    Press press(const Geometry& g, int x, int y, Mods mods, int& hit);
    bool drag(const Geometry& g, int x, int y, Mods mods);
    bool release(const Geometry& g);
    bool doubleClick(const Geometry& g, int x, int y);

    void selectAll();
    void nudge(const Geometry& g, int dTicks, int dSemis, int dVel);
    void closeNudge() { nudgeOpen_ = false; }
    void printGroove(const Geometry& g, const swing::Groove& groove);
    void quantise(const Geometry& g, int gridTicks);
    void deleteSelection(const Geometry& g);
    void copySelection(const Geometry& g, bool cut);
    void paste(const Geometry& g, double playheadTick);
    void duplicate(const Geometry& g);
    void split(const Geometry& g, int noteIndex, int atTick);
    void marqueeSelect(const Geometry& g, const Rect& area, bool additive);
    void ccMarqueeSelect(const Geometry& g, const Rect& area, bool additive);

    int selectedNotesColour() const;
    void colourSelectedNotes(const Geometry& g, int colour, bool asUndoStep);
    int selectedCCsColour() const;
    void colourSelectedCCs(int colour, bool asUndoStep);
    void deleteSelectedCCs();
    void clearLane();

    void scrollPitch(int step, int rows);
    void fitPitch(int rows);
    void followRecording(int rows);

    static const std::vector<NoteEvent>& clipboard() { return clipboard_; }

private:
    void commit(const Geometry& g, const std::vector<NoteEvent>& n) { clip_.setNotes(n, g.duration); }
    bool pressLane(const Geometry& g, int x, int y, Mods mods, Press& result);
    void applyVelocityLane(const Geometry& g, int x, int y);
    void applyCCLane(const Geometry& g, int x, int y);
    bool dragNotes(const Geometry& g, int x, int y, int tick);

    ClipNotes& clip_;
    Gesture gesture_ = Gesture::None;
    std::vector<NoteEvent> gestureNotes_;
    std::vector<NoteEvent> gestureBase_;
    std::vector<CCEvent> gestureCCs_;
    int gestureIndex_ = -1;
    int gestureStartTick_ = 0, gestureStartPitch_ = 0, gestureStartVel_ = 100;
    int gestureTickOffset_ = 0;
    int dragX_ = 0, dragY_ = 0;
    Rect marquee_;
    int keyPitch_ = -1;
    bool nudgeOpen_ = false;
    static std::vector<NoteEvent> clipboard_;
};

}
