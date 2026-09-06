#include "gui/TracksPane.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <map>

#include "gui/LookAndFeel.h"
#include "gui/NoteEdit.h"

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {
constexpr int kNoteEdge = noteedit::kEdgePx;
constexpr int kMinNoteTicks = noteedit::kMinTicks;
}

juce::Rectangle<float> TracksPane::noteBounds(int clipStart, const NoteEvent& n) const {
    const auto rp = rollPlot();
    const float x = tickToX(clipStart + n.tick);
    const float w = std::max(2.0f, tickToX(clipStart + n.tick
                                           + std::max(1, n.lengthTicks)) - x);
    return {x, rp.yFor(n.pitch), w, std::max(2.0f, rp.hFor(n.pitch) - 1.0f)};
}

int TracksPane::noteAt(juce::Point<int> p, int& clip, int& index,
                       bool& leftEdge, bool& rightEdge) const {
    clip = index = -1;
    leftEdge = rightEdge = false;
    const auto clips = host_.clips().list(trackNode_);
    for (int c = (int) clips.size() - 1; c >= 0; --c) {
        const auto notes = host_.clips().notes(trackNode_, c);
        for (int i = (int) notes.size() - 1; i >= 0; --i) {
            const auto b = noteBounds(clips[(size_t) c].startTick, notes[(size_t) i]);
            if (!b.expanded(0.0f, 1.0f).contains(p.toFloat())) continue;
            clip = c;
            index = i;
            const auto g = noteedit::grabAt(b, p.toFloat());
            leftEdge = g == noteedit::Grab::LeftEdge;
            rightEdge = g == noteedit::Grab::RightEdge;
            return i;
        }
    }
    return -1;
}

int TracksPane::clipOwning(int absTick) const {
    const auto clips = host_.clips().list(trackNode_);
    for (int c = (int) clips.size() - 1; c >= 0; --c)
        if (absTick >= clips[(size_t) c].startTick
            && absTick < clips[(size_t) c].startTick + clips[(size_t) c].lengthTicks)
            return c;
    return -1;
}

void TracksPane::snapshotNotes() {
    rollBase_.clear();
    const auto clips = host_.clips().list(trackNode_);
    for (const auto& ci : clips)
        rollBase_[ci.index] = host_.clips().notes(trackNode_, ci.index);
    rollSelBase_ = selNotes_;
}

void TracksPane::commitNotes(std::map<int, std::vector<NoteEvent>> perClip,
                             const std::vector<NoteKey>& keep) {
    for (auto& [clip, notes] : perClip) {
        std::stable_sort(notes.begin(), notes.end(),
                         [](const NoteEvent& a, const NoteEvent& b) { return a.tick < b.tick; });
        host_.clips().setNotes(trackNode_, clip, notes, 0);
    }
    selNotes_.clear();
    for (const auto& k : keep) {
        const auto it = perClip.find(k.clip);
        if (it == perClip.end()) continue;
        for (int i = 0; i < (int) it->second.size(); ++i)
            if (it->second[(size_t) i].tick == k.tick
                && it->second[(size_t) i].pitch == k.pitch) selNotes_.insert({k.clip, i});
    }
    repaint();
}

void TracksPane::applyNoteDrag(const juce::MouseEvent& e) {
    if (rollBase_.empty()) return;
    const auto clips = host_.clips().list(trackNode_);
    const auto rp = rollPlot();
    const bool alt = e.mods.isAltDown();

    const int tickNow = std::max(0, xToTick((float) e.x));
    int dTicks = tickNow - rollAnchorTick_;
    if (!alt && rollDrag_ != RollDrag::Velocity) {
        const double grid = gridBeats() * Pattern::kTicksPerBeat;
        dTicks = (int) std::llround(dTicks / grid) * (int) grid;
    }
    const int dPitch = rp.pitchAt((float) e.y) - rollAnchorPitch_;
    const int dVel = rollAnchorVel_ > 0
        ? (int) std::lround((rollAnchor_.y - e.y) * 1.5f) : 0;

    std::map<int, std::vector<NoteEvent>> out;
    for (const auto& ci : clips) out[ci.index] = {};
    std::vector<NoteKey> keep;

    for (const auto& [clip, notes] : rollBase_) {
        int start = 0;
        for (const auto& ci : clips) if (ci.index == clip) start = ci.startTick;
        for (int i = 0; i < (int) notes.size(); ++i) {
            if (rollSelBase_.count({clip, i}) == 0) {
                out[clip].push_back(notes[(size_t) i]);
                continue;
            }
            NoteEvent n = notes[(size_t) i];
            int home = clip, at = start + n.tick;
            switch (rollDrag_) {
                case RollDrag::Move:
                    at += dTicks;
                    n.pitch = juce::jlimit(0, kMidiMax, n.pitch + dPitch);
                    break;
                case RollDrag::ResizeR:
                    n.lengthTicks = std::max(kMinNoteTicks, n.lengthTicks + dTicks);
                    break;
                case RollDrag::ResizeL:
                    at += dTicks;
                    n.lengthTicks = std::max(kMinNoteTicks, n.lengthTicks - dTicks);
                    break;
                case RollDrag::Velocity:
                    n.velocity = juce::jlimit(1, kMidiMax, n.velocity + dVel);
                    break;
                default: break;
            }
            if (rollDrag_ == RollDrag::Move || rollDrag_ == RollDrag::ResizeL) {
                if (const int owner = clipOwning(at); owner >= 0) home = owner;
                else { home = clip; at = start + notes[(size_t) i].tick; n = notes[(size_t) i]; }
            }
            int hs = 0, hl = 0;
            for (const auto& ci : clips)
                if (ci.index == home) { hs = ci.startTick; hl = ci.lengthTicks; }
            n.tick = std::max(0, at - hs);
            n.lengthTicks = juce::jlimit(kMinNoteTicks, std::max(kMinNoteTicks, hl - n.tick),
                                         n.lengthTicks);
            out[home].push_back(n);
            keep.push_back({home, n.tick, n.pitch});
        }
    }
    commitNotes(std::move(out), keep);
}

void TracksPane::applyVelLaneEdit(juce::Point<int> p) {
    const int absTick = std::max(0, xToTick((float) p.x));
    const int slack = std::max(1, xToTick((float) p.x + 6.0f) - absTick);
    int bestClip = -1, bestIdx = -1, bestDist = INT_MAX;
    for (const auto& ci : host_.clips().list(trackNode_)) {
        const auto notes = host_.clips().notes(trackNode_, ci.index);
        for (int i = 0; i < (int) notes.size(); ++i) {
            const int d = std::abs(ci.startTick + notes[(size_t) i].tick - absTick);
            if (d <= slack && d < bestDist) {
                bestDist = d;
                bestClip = ci.index;
                bestIdx = i;
            }
        }
    }
    if (bestIdx < 0) return;
    auto notes = host_.clips().notes(trackNode_, bestClip);
    if (bestIdx >= (int) notes.size()) return;
    const int vel = velAtY(p.y);
    notes[(size_t) bestIdx].velocity = vel;
    host_.clips().setNotes(trackNode_, bestClip, notes, 0);
    velShowX_ = p.x;
    velShowVal_ = vel;
    repaint();
}

void TracksPane::applyVelLaneLine(juce::Point<int> a, juce::Point<int> b) {
    if (a.x > b.x) std::swap(a, b);
    const int tickA = std::max(0, xToTick((float) a.x));
    const int tickB = std::max(tickA, xToTick((float) b.x));
    const int velA = velAtY(a.y);
    const int velB = velAtY(b.y);
    const int slack = std::max(1, xToTick((float) a.x + 4.0f) - tickA);
    for (const auto& ci : host_.clips().list(trackNode_)) {
        auto notes = host_.clips().notes(trackNode_, ci.index);
        bool changed = false;
        for (auto& n : notes) {
            const int at = ci.startTick + n.tick;
            if (at < tickA - slack || at > tickB + slack) continue;
            const double t = tickB > tickA
                ? juce::jlimit(0.0, 1.0, (at - tickA) / (double) (tickB - tickA))
                : 0.0;
            const int vel =
                juce::jlimit(1, kMidiMax, (int) std::lround(velA + (velB - velA) * t));
            if (n.velocity != vel) { n.velocity = vel; changed = true; }
        }
        if (changed) host_.clips().setNotes(trackNode_, ci.index, notes, 0);
    }
    velShowX_ = b.x;
    velShowVal_ = velB;
    repaint();
}

void TracksPane::quantiseSelectedNotes(int gridTicks) {
    if (selNotes_.empty() || gridTicks <= 0) return;
    host_.pushUndo();
    const auto clips = host_.clips().list(trackNode_);
    for (const auto& ci : clips) {
        auto notes = host_.clips().notes(trackNode_, ci.index);
        bool touched = false;
        for (int i = 0; i < (int) notes.size(); ++i) {
            if (selNotes_.count({ci.index, i}) == 0) continue;
            const int was = notes[(size_t) i].tick;
            const int now = (int) std::llround(was / (double) gridTicks) * gridTicks;
            notes[(size_t) i].tick =
                juce::jlimit(0, std::max(0, ci.lengthTicks - 1), now);
            touched = touched || notes[(size_t) i].tick != was;
        }
        if (touched) host_.clips().setNotes(trackNode_, ci.index, notes, 0);
    }
    selNotes_.clear();
    rebuild();
}

bool TracksPane::deleteSelectedNotes() {
    if (selNotes_.empty()) return false;
    host_.pushUndo();
    const auto clips = host_.clips().list(trackNode_);
    for (const auto& ci : clips) {
        auto notes = host_.clips().notes(trackNode_, ci.index);
        std::vector<NoteEvent> kept;
        for (int i = 0; i < (int) notes.size(); ++i)
            if (selNotes_.count({ci.index, i}) == 0) kept.push_back(notes[(size_t) i]);
        if (kept.size() != notes.size())
            host_.clips().setNotes(trackNode_, ci.index, kept, 0);
    }
    selNotes_.clear();
    repaint();
    return true;
}

void TracksPane::nudgeNotes(int dTicks, int dSemis, int dVel) {
    if (selNotes_.empty()) return;
    if (!nudgeRunUndoOpen_) { host_.pushUndo(); nudgeRunUndoOpen_ = true; }
    snapshotNotes();
    const auto clips = host_.clips().list(trackNode_);
    std::map<int, std::vector<NoteEvent>> out;
    for (const auto& ci : clips) out[ci.index] = {};
    std::vector<NoteKey> keep;
    for (const auto& [clip, notes] : rollBase_) {
        int start = 0, len = 0;
        for (const auto& ci : clips) if (ci.index == clip) { start = ci.startTick; len = ci.lengthTicks; }
        for (int i = 0; i < (int) notes.size(); ++i) {
            if (rollSelBase_.count({clip, i}) == 0) {
                out[clip].push_back(notes[(size_t) i]);
                continue;
            }
            NoteEvent n = notes[(size_t) i];
            int home = clip, at = start + n.tick;
            n.pitch = juce::jlimit(0, kMidiMax, n.pitch + dSemis);
            n.velocity = juce::jlimit(1, kMidiMax, n.velocity + dVel);
            at += dTicks;
            if (dTicks != 0) {
                if (const int owner = clipOwning(at); owner >= 0) home = owner;
                else at = start + n.tick;
            }
            int hs = start, hl = len;
            for (const auto& ci : clips)
                if (ci.index == home) { hs = ci.startTick; hl = ci.lengthTicks; }
            n.tick = std::max(0, at - hs);
            n.lengthTicks = juce::jlimit(kMinNoteTicks, std::max(kMinNoteTicks, hl - n.tick),
                                         n.lengthTicks);
            out[home].push_back(n);
            keep.push_back({home, n.tick, n.pitch});
        }
    }
    commitNotes(std::move(out), keep);
}

void TracksPane::selectAllNotes() {
    selNotes_.clear();
    for (const auto& ci : host_.clips().list(trackNode_))
        for (int i = 0; i < (int) host_.clips().notes(trackNode_, ci.index).size(); ++i)
            selNotes_.insert({ci.index, i});
    repaint();
}

void TracksPane::eraseNoteUnder(juce::Point<int> p) {
    int clip = -1, index = -1;
    bool l = false, r = false;
    if (noteAt(p, clip, index, l, r) < 0) return;
    auto notes = host_.clips().notes(trackNode_, clip);
    if (index >= (int) notes.size()) return;
    notes.erase(notes.begin() + index);
    host_.clips().setNotes(trackNode_, clip, notes, 0);
    selNotes_.clear();
    repaint();
}

void TracksPane::copySelectedNotes() {
    if (selNotes_.empty()) return;
    noteClipboard_.clear();
    int base = INT_MAX, end = 0;
    const auto clips = host_.clips().list(trackNode_);
    for (const auto& ci : clips) {
        const auto notes = host_.clips().notes(trackNode_, ci.index);
        for (int i = 0; i < (int) notes.size(); ++i) {
            if (selNotes_.count({ci.index, i}) == 0) continue;
            const int at = ci.startTick + notes[(size_t) i].tick;
            base = std::min(base, at);
            end = std::max(end, at + std::max(1, notes[(size_t) i].lengthTicks));
        }
    }
    if (base == INT_MAX) return;
    noteClipboardSpan_ = std::max(1, end - base);
    for (const auto& ci : clips) {
        const auto notes = host_.clips().notes(trackNode_, ci.index);
        for (int i = 0; i < (int) notes.size(); ++i)
            if (selNotes_.count({ci.index, i}) != 0)
                noteClipboard_.push_back({ci.startTick + notes[(size_t) i].tick - base,
                                          notes[(size_t) i]});
    }
}

bool TracksPane::pasteNotes(int atTick) {
    if (noteClipboard_.empty()) return false;
    host_.pushUndo();
    const auto clips = host_.clips().list(trackNode_);
    std::map<int, std::vector<NoteEvent>> out;
    for (const auto& ci : clips) out[ci.index] = host_.clips().notes(trackNode_, ci.index);
    std::vector<NoteKey> keep;
    bool any = false;
    for (const auto& c : noteClipboard_) {
        const int at = std::max(0, atTick + c.tick);
        const int home = clipOwning(at);
        if (home < 0) continue;
        int hs = 0, hl = 0;
        for (const auto& ci : clips)
            if (ci.index == home) { hs = ci.startTick; hl = ci.lengthTicks; }
        NoteEvent n = c.n;
        n.tick = at - hs;
        n.lengthTicks = juce::jlimit(1, std::max(1, hl - n.tick), n.lengthTicks);
        out[home].push_back(n);
        keep.push_back({home, n.tick, n.pitch});
        any = true;
    }
    if (!any) return false;
    commitNotes(std::move(out), keep);
    return true;
}

bool TracksPane::duplicateSelectedNotes() {
    if (selNotes_.empty()) return false;
    copySelectedNotes();
    int base = INT_MAX;
    for (const auto& ci : host_.clips().list(trackNode_)) {
        const auto notes = host_.clips().notes(trackNode_, ci.index);
        for (int i = 0; i < (int) notes.size(); ++i)
            if (selNotes_.count({ci.index, i}) != 0)
                base = std::min(base, ci.startTick + notes[(size_t) i].tick);
    }
    if (base == INT_MAX) return false;
    return pasteNotes(base + noteClipboardSpan_);
}

bool TracksPane::mouseDownRollGrid(const juce::MouseEvent& e, juce::Point<int> p) {
    const auto rp = rollPlot();
    if (rp.usable && rollShowsVelocity() && !e.mods.isPopupMenu() && p.x >= kStripW
        && std::abs(p.y - (fieldBottom() - velH_)) <= 3) {
        rollDrag_ = RollDrag::VelDivider;
        return true;
    }
    if (rp.usable && rollShowsVelocity() && !e.mods.isPopupMenu() && p.x >= kStripW
        && p.y >= fieldBottom() - velH_ && p.y < fieldBottom()) {
        host_.pushUndo();
        rollDrag_ = RollDrag::VelLane;
        velLine_ = effectiveTool() == Tool::Line || e.mods.isShiftDown();
        velAnchor_ = p;
        if (velLine_) applyVelLaneLine(p, p);
        else applyVelLaneEdit(p);
        return true;
    }
    if (!rp.usable || !rollField().contains(p)) return false;
    nudgeRunUndoOpen_ = false;

    int clip = -1, index = -1;
    bool leftEdge = false, rightEdge = false;
    noteAt(p, clip, index, leftEdge, rightEdge);
    const int tick = std::max(0, xToTick((float) p.x));
    const int pitch = juce::jlimit(0, kMidiMax, rp.pitchAt((float) p.y));
    rollAnchor_ = p;
    rollAnchorTick_ = tick;
    rollAnchorPitch_ = pitch;
    rollAnchorVel_ = 0;

    if (e.mods.isPopupMenu()) {
        if (index >= 0 && selNotes_.count({clip, index}) == 0) {
            selNotes_.clear();
            selNotes_.insert({clip, index});
            repaint();
        }
        showNoteMenu(localPointToGlobal(p));
        return true;
    }

    const auto tool = effectiveTool();
    if (tool == Tool::Eraser) {
        host_.pushUndo();
        rollDrag_ = RollDrag::Erase;
        eraseNoteUnder(p);
        return true;
    }
    if (tool == Tool::Scissors) {
        if (index < 0) return true;
        host_.pushUndo();
        auto notes = host_.clips().notes(trackNode_, clip);
        int start = 0;
        for (const auto& ci : host_.clips().list(trackNode_))
            if (ci.index == clip) start = ci.startTick;
        const int cut = (int) std::llround(snapBeats(xToBeat((float) p.x), e.mods.isAltDown())
                                           * Pattern::kTicksPerBeat) - start;
        auto& n = notes[(size_t) index];
        if (cut > n.tick + kMinNoteTicks && cut < n.tick + n.lengthTicks - kMinNoteTicks) {
            NoteEvent tail = n;
            tail.tick = cut;
            tail.lengthTicks = n.tick + n.lengthTicks - cut;
            n.lengthTicks = cut - n.tick;
            notes.push_back(tail);
            host_.clips().setNotes(trackNode_, clip, notes, 0);
        }
        selNotes_.clear();
        repaint();
        return true;
    }
    if (tool == Tool::Draw) {
        const int owner = clipOwning(tick);
        if (owner < 0) return true;
        host_.pushUndo();
        int start = 0;
        for (const auto& ci : host_.clips().list(trackNode_))
            if (ci.index == owner) start = ci.startTick;
        const int at = (int) std::llround(snapBeats(xToBeat((float) p.x), e.mods.isAltDown())
                                          * Pattern::kTicksPerBeat);
        auto notes = host_.clips().notes(trackNode_, owner);
        NoteEvent n;
        n.tick = std::max(0, at - start);
        n.pitch = pitch;
        n.lengthTicks = std::max(kMinNoteTicks,
                                 (int) std::llround(gridBeats() * Pattern::kTicksPerBeat));
        n.velocity = 100;
        notes.push_back(n);
        host_.clips().setNotes(trackNode_, owner, notes, 0);
        selNotes_.clear();
        for (int i = 0; i < (int) notes.size(); ++i)
            if (notes[(size_t) i].tick == n.tick && notes[(size_t) i].pitch == n.pitch)
                selNotes_.insert({owner, i});
        rollAnchorTick_ = at;
        snapshotNotes();
        rollDrag_ = RollDrag::ResizeR;
        repaint();
        return true;
    }

    if (index < 0) {
        if (!e.mods.isShiftDown()) selNotes_.clear();
        rollMarquee_ = juce::Rectangle<int>(p, p);
        rollDrag_ = RollDrag::Marquee;
        repaint();
        return true;
    }
    const std::pair<int, int> hit{clip, index};
    if (e.mods.isShiftDown()) {
        if (!selNotes_.insert(hit).second) selNotes_.erase(hit);
        repaint();
        return true;
    }
    if (selNotes_.count(hit) == 0) { selNotes_.clear(); selNotes_.insert(hit); }
    host_.pushUndo();
    snapshotNotes();
    if (e.mods.isAltDown()) {
        rollAnchorVel_ = 1;
        rollDrag_ = RollDrag::Velocity;
    } else {
        rollDrag_ = leftEdge ? RollDrag::ResizeL
                  : rightEdge ? RollDrag::ResizeR : RollDrag::Move;
    }
    repaint();
    return true;
}

void TracksPane::mouseDragRoll(const juce::MouseEvent& e) {
    switch (rollDrag_) {
        case RollDrag::None: return;
        case RollDrag::Erase: eraseNoteUnder(e.getPosition()); return;
        case RollDrag::VelDivider:
            velH_ = juce::jlimit(28, 160, fieldBottom() - e.y);
            repaint();
            return;
        case RollDrag::VelLane:
            if (velLine_) applyVelLaneLine(velAnchor_, e.getPosition());
            else applyVelLaneEdit(e.getPosition());
            return;
        case RollDrag::Marquee: {
            rollMarquee_ = juce::Rectangle<int>(rollAnchor_, e.getPosition());
            selNotes_.clear();
            for (const auto& ci : host_.clips().list(trackNode_)) {
                const auto notes = host_.clips().notes(trackNode_, ci.index);
                for (int i = 0; i < (int) notes.size(); ++i)
                    if (rollMarquee_.toFloat().intersects(
                            noteBounds(ci.startTick, notes[(size_t) i])))
                        selNotes_.insert({ci.index, i});
            }
            repaint();
            return;
        }
        default: applyNoteDrag(e); return;
    }
}

void TracksPane::mouseUpRoll() {
    if (rollKeyNote_ >= 0) {
        host_.injectLiveMidi(juce::MidiMessage::noteOff(1, rollKeyNote_));
        rollKeyNote_ = -1;
        repaintRollKeys();
    }
    rollDrag_ = RollDrag::None;
    velLine_ = false;
    velShowX_ = -1;
    rollMarquee_ = {};
    rollBase_.clear();
    rollSelBase_.clear();
    rollAnchorVel_ = 0;
    repaint();
}

void TracksPane::paintRollSelection(juce::Graphics& g) {
    if (mode_ != Mode::Track) return;
    if (rollDrag_ == RollDrag::Marquee && !rollMarquee_.isEmpty()) {
        g.setColour(Palette::accent.withAlpha(0.14f));
        g.fillRect(rollMarquee_);
        g.setColour(Palette::accent);
        g.drawRect(rollMarquee_, 1);
    }
    if (selNotes_.empty()) return;
    const auto f = rollField();
    g.saveState();
    g.reduceClipRegion(f);
    g.setColour(Palette::text);
    for (const auto& ci : host_.clips().list(trackNode_)) {
        const auto notes = host_.clips().notes(trackNode_, ci.index);
        for (int i = 0; i < (int) notes.size(); ++i) {
            if (selNotes_.count({ci.index, i}) == 0) continue;
            const auto b = noteBounds(ci.startTick, notes[(size_t) i]);
            if (b.getRight() < (float) kStripW || b.getX() > (float) getWidth()) continue;
            g.drawRoundedRectangle(b.expanded(1.0f, 1.0f), 2.0f, 1.2f);
        }
    }
    g.restoreState();
}

}
