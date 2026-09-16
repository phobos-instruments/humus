// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/tracks/TrackRollView.h"
#include <algorithm>
#include <climits>
#include <cmath>
#include <iterator>
#include "core/timeline/ClipOps.h"
#include "gui/common/Localisation.h"
#include "gui/host/EngineHostMidiControl.h"
#include "gui/host/TracksHost.h"
#include "gui/pianoroll/NoteEdit.h"
#include "gui/tracks/CutGuide.h"
#include "gui/tracks/QuantiseMenu.h"
#include "io/PatchDocument.h"
#include "hum/dsp/DspMath.h"

namespace hum {

namespace {
constexpr int kMinNoteTicks = noteedit::kMinTicks;
}

void TrackRollView::applyNoteDrag(const juce::MouseEvent& e) {
    const auto pe = toPane(e.getPosition());
    if (base_.empty()) return;
    const auto clips = host().clips().list(node_);
    const auto rp = rollPlot();
    const bool alt = e.mods.isAltDown();

    const int tickNow = std::max(0, xToTick((float) pe.x));
    int dTicks = tickNow - anchorTick_;
    if (!alt && drag_ != Drag::Velocity) {
        const double grid = ctx_.gridBeats() * Pattern::kTicksPerBeat;
        dTicks = (int) std::llround(dTicks / grid) * (int) grid;
    }
    const int dPitch = rp.pitchAt((float) pe.y) - anchorPitch_;
    const int dVel = anchorVel_ > 0
        ? (int) std::lround((anchor_.y - pe.y) * 1.5f) : 0;

    std::map<int, std::vector<NoteEvent>> out;
    for (const auto& ci : clips) out[ci.index] = {};
    std::vector<NoteKey> keep;

    for (const auto& [clip, notes] : base_) {
        int start = 0;
        for (const auto& ci : clips) if (ci.index == clip) start = ci.startTick;
        for (int i = 0; i < (int) notes.size(); ++i) {
            if (selBase_.count({clip, i}) == 0) {
                out[clip].push_back(notes[(size_t) i]);
                continue;
            }
            NoteEvent n = notes[(size_t) i];
            int home = clip, at = start + n.tick;
            switch (drag_) {
                case Drag::Move:
                    at += dTicks;
                    n.pitch = juce::jlimit(0, kMidiMax, n.pitch + dPitch);
                    break;
                case Drag::ResizeR:
                    n.lengthTicks = std::max(kMinNoteTicks, n.lengthTicks + dTicks);
                    break;
                case Drag::ResizeL:
                    at += dTicks;
                    n.lengthTicks = std::max(kMinNoteTicks, n.lengthTicks - dTicks);
                    break;
                case Drag::Velocity:
                    n.velocity = juce::jlimit(1, kMidiMax, n.velocity + dVel);
                    break;
                default: break;
            }
            if (drag_ == Drag::Move || drag_ == Drag::ResizeL) {
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

void TrackRollView::applyVelLaneEdit(juce::Point<int> p) {
    const int absTick = std::max(0, xToTick((float) p.x));
    const int slack = std::max(1, xToTick((float) p.x + 6.0f) - absTick);
    int bestClip = -1, bestIdx = -1, bestDist = INT_MAX;
    for (const auto& ci : host().clips().list(node_)) {
        const auto notes = host().clips().notes(node_, ci.index);
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
    auto notes = host().clips().notes(node_, bestClip);
    if (bestIdx >= (int) notes.size()) return;
    const int vel = velAtY(p.y);
    notes[(size_t) bestIdx].velocity = vel;
    host().clips().setNotes(node_, bestClip, notes, 0);
    velShowX_ = p.x;
    velShowVal_ = vel;
    repaint();
}

void TrackRollView::applyVelLaneLine(juce::Point<int> a, juce::Point<int> b) {
    if (a.x > b.x) std::swap(a, b);
    const int tickA = std::max(0, xToTick((float) a.x));
    const int tickB = std::max(tickA, xToTick((float) b.x));
    const int velA = velAtY(a.y);
    const int velB = velAtY(b.y);
    const int slack = std::max(1, xToTick((float) a.x + 4.0f) - tickA);
    for (const auto& ci : host().clips().list(node_)) {
        auto notes = host().clips().notes(node_, ci.index);
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
        if (changed) host().clips().setNotes(node_, ci.index, notes, 0);
    }
    velShowX_ = b.x;
    velShowVal_ = velB;
    repaint();
}

void TrackRollView::commitNotes(std::map<int, std::vector<NoteEvent>> perClip,
                             const std::vector<NoteKey>& keep) {
    for (auto& [clip, notes] : perClip) {
        std::stable_sort(notes.begin(), notes.end(),
                         [](const NoteEvent& a, const NoteEvent& b) { return a.tick < b.tick; });
        host().clips().setNotes(node_, clip, notes, 0);
    }
    sel_.clear();
    for (const auto& k : keep) {
        const auto it = perClip.find(k.clip);
        if (it == perClip.end()) continue;
        for (int i = 0; i < (int) it->second.size(); ++i)
            if (it->second[(size_t) i].tick == k.tick
                && it->second[(size_t) i].pitch == k.pitch) sel_.insert({k.clip, i});
    }
    repaint();
}

void TrackRollView::copySelectedNotes() {
    if (sel_.empty()) return;
    clipboard_.clear();
    int base = INT_MAX, end = 0;
    const auto clips = host().clips().list(node_);
    for (const auto& ci : clips) {
        const auto notes = host().clips().notes(node_, ci.index);
        for (int i = 0; i < (int) notes.size(); ++i) {
            if (sel_.count({ci.index, i}) == 0) continue;
            const int at = ci.startTick + notes[(size_t) i].tick;
            base = std::min(base, at);
            end = std::max(end, at + std::max(1, notes[(size_t) i].lengthTicks));
        }
    }
    if (base == INT_MAX) return;
    clipboardSpan_ = std::max(1, end - base);
    for (const auto& ci : clips) {
        const auto notes = host().clips().notes(node_, ci.index);
        for (int i = 0; i < (int) notes.size(); ++i)
            if (sel_.count({ci.index, i}) != 0)
                clipboard_.push_back({ci.startTick + notes[(size_t) i].tick - base,
                                          notes[(size_t) i]});
    }
}

bool TrackRollView::deleteSelectedNotes() {
    if (sel_.empty()) return false;
    host().pushUndo();
    const auto clips = host().clips().list(node_);
    for (const auto& ci : clips) {
        auto notes = host().clips().notes(node_, ci.index);
        std::vector<NoteEvent> kept;
        for (int i = 0; i < (int) notes.size(); ++i)
            if (sel_.count({ci.index, i}) == 0) kept.push_back(notes[(size_t) i]);
        if (kept.size() != notes.size())
            host().clips().setNotes(node_, ci.index, kept, 0);
    }
    sel_.clear();
    repaint();
    return true;
}

bool TrackRollView::duplicateSelectedNotes() {
    if (sel_.empty()) return false;
    copySelectedNotes();
    int base = INT_MAX;
    for (const auto& ci : host().clips().list(node_)) {
        const auto notes = host().clips().notes(node_, ci.index);
        for (int i = 0; i < (int) notes.size(); ++i)
            if (sel_.count({ci.index, i}) != 0)
                base = std::min(base, ci.startTick + notes[(size_t) i].tick);
    }
    if (base == INT_MAX) return false;
    return pasteNotes(base + clipboardSpan_);
}

void TrackRollView::eraseNoteUnder(juce::Point<int> p) {
    int clip = -1, index = -1;
    bool l = false, r = false;
    if (noteAt(p, clip, index, l, r) < 0) return;
    auto notes = host().clips().notes(node_, clip);
    if (index >= (int) notes.size()) return;
    notes.erase(notes.begin() + index);
    host().clips().setNotes(node_, clip, notes, 0);
    sel_.clear();
    repaint();
}

void TrackRollView::nudgeNotes(int dTicks, int dSemis, int dVel) {
    if (sel_.empty()) return;
    if (!nudgeRunUndoOpen_) { host().pushUndo(); nudgeRunUndoOpen_ = true; }
    snapshotNotes();
    const auto clips = host().clips().list(node_);
    std::map<int, std::vector<NoteEvent>> out;
    for (const auto& ci : clips) out[ci.index] = {};
    std::vector<NoteKey> keep;
    for (const auto& [clip, notes] : base_) {
        int start = 0, len = 0;
        for (const auto& ci : clips) if (ci.index == clip) { start = ci.startTick; len = ci.lengthTicks; }
        for (int i = 0; i < (int) notes.size(); ++i) {
            if (selBase_.count({clip, i}) == 0) {
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

bool TrackRollView::pasteNotes(int atTick) {
    if (clipboard_.empty()) return false;
    host().pushUndo();
    const auto clips = host().clips().list(node_);
    std::map<int, std::vector<NoteEvent>> out;
    for (const auto& ci : clips) out[ci.index] = host().clips().notes(node_, ci.index);
    std::vector<NoteKey> keep;
    bool any = false;
    for (const auto& c : clipboard_) {
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

void TrackRollView::quantiseSelectedNotes(int gridTicks) {
    if (sel_.empty() || gridTicks <= 0) return;
    host().pushUndo();
    const auto clips = host().clips().list(node_);
    for (const auto& ci : clips) {
        auto notes = host().clips().notes(node_, ci.index);
        bool touched = false;
        for (int i = 0; i < (int) notes.size(); ++i) {
            if (sel_.count({ci.index, i}) == 0) continue;
            const int was = notes[(size_t) i].tick;
            const int now = (int) std::llround(was / (double) gridTicks) * gridTicks;
            notes[(size_t) i].tick =
                juce::jlimit(0, std::max(0, ci.lengthTicks - 1), now);
            touched = touched || notes[(size_t) i].tick != was;
        }
        if (touched) host().clips().setNotes(node_, ci.index, notes, 0);
    }
    sel_.clear();
    ctx_.rebuildRows();
}

void TrackRollView::selectAllNotes() {
    sel_.clear();
    for (const auto& ci : host().clips().list(node_))
        for (int i = 0; i < (int) host().clips().notes(node_, ci.index).size(); ++i)
            sel_.insert({ci.index, i});
    repaint();
}

void TrackRollView::snapshotNotes() {
    base_.clear();
    const auto clips = host().clips().list(node_);
    for (const auto& ci : clips)
        base_[ci.index] = host().clips().notes(node_, ci.index);
    selBase_ = sel_;
}

void TrackRollView::showNoteMenu(juce::Point<int> screenPos) {
    enum { kNoteDelete = 1, kNoteLouder, kNoteSofter, kNoteSelectAll };
    juce::PopupMenu m;
    const int n = (int) sel_.size();
    const auto count = n == 1 ? juce::String("note") : juce::String(n) + " notes";
    m.addSectionHeader(n > 0 ? count : juce::String(tr("tracks-pane-menu.no-notes-selected", "No notes selected")));
    m.addItem(kNoteSelectAll, tr("tracks-pane-menu.select-all", "Select All"), true);
    if (n > 0) {
        m.addSeparator();
        m.addSubMenu(tr("tracks-pane-menu.quantise-to", "Quantise to"), quantise::menu(ctx_.gridBeats()));
        m.addSeparator();
        m.addItem(kNoteLouder, tr("tracks-pane-menu.louder", "Louder"));
        m.addItem(kNoteSofter, tr("tracks-pane-menu.softer", "Softer"));
        m.addSeparator();
        m.addItem(kNoteDelete, tr("tracks-pane-menu.delete", "Delete"));
    }
    m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({screenPos, screenPos}),
                    [this](int res) {
        if (res == 0) return;
        if (res == kNoteSelectAll) { selectAllNotes(); return; }
        if (const int q = quantise::ticksFor(res, ctx_.gridBeats()); q > 0) {
            quantiseSelectedNotes(q);
            return;
        }
        if (res == kNoteDelete) { deleteSelectedNotes(); ctx_.rebuildRows(); return; }
        if (res == kNoteLouder) nudgeNotes(0, 0, 10);
        if (res == kNoteSofter) nudgeNotes(0, 0, -10);
    });
}

}
