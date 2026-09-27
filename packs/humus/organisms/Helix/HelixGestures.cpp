// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Helix/Helix.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace hum {

void Helix::clearStrand(Strand& s) {
    std::fill(s.chunkPeak.begin(), s.chunkPeak.end(), 0.0f);
    s.peakChunkAt = -1;
    s.waveDirty = true;
    s.len = s.recCount = 0;
    s.pos = 0.0;
    s.recBpm = 0.0;
    s.state = SState::Empty;
    s.dubStart = s.dubWritten = s.snapCursor = 0;
    s.lastWrite = -1;
    s.dubHead = 0.0;
    s.dubLag = s.dubWarm = 0;
    s.dubTail = -1;
    s.dubFresh = false;
    s.snapDone = true;
    s.undoReady = s.redo = false;
    s.layers = 0;
    s.pending = Pending::None;
    s.pendingLate = 0;
}

void Helix::closeLoop(Strand& s, std::int64_t late, bool thenPlay) {
    late = std::clamp<std::int64_t>(late, 0, s.recCount > 1 ? s.recCount - 1 : 0);
    s.len = std::max<std::int64_t>(1, s.recCount - late);
    s.pos = thenPlay ? (double) (late % s.len) : 0.0;
    s.recBpm = tempoNow_;
    s.layers = 1;
    s.undoReady = s.redo = false;
    s.snapDone = true;
    s.state = thenPlay ? SState::Play : SState::Stopped;
}

void Helix::beginDub(Strand& s) {
    s.dubStart = (std::int64_t) s.pos;
    s.dubHead = s.pos;
    s.dubDir = s.reverse ? -1 : 1;
    s.dubWritten = 0;
    s.lastWrite = -1;
    s.snapCursor = 0;
    s.snapDone = s.len <= 0;
    s.undoReady = s.redo = false;
    s.dubLag = s.stretch ? dubLag_ : 0;
    s.dubWarm = s.dubLag;
    s.dubTail = -1;
    s.dubFresh = s.dubLag > 0;
    ++s.layers;
    s.state = SState::Dub;
}

void Helix::finishDub(Strand& s) {
    if (s.dubLag <= 0) leaveDub(s);
    else if (s.dubTail < 0) s.dubTail = s.dubLag;
}

void Helix::leaveDub(Strand& s) {
    s.state = SState::Play;
    s.dubTail = -1;
}

void Helix::recPress(Strand& s, std::int64_t late) {
    switch (s.state) {
        case SState::Empty: {
            late = std::clamp<std::int64_t>(late, 0, maxLoopSamples_ - 1);
            for (auto& ch : s.buf) std::fill(ch.begin(), ch.begin() + (size_t) late, 0.0f);
            s.recCount = late;
            s.layers = 0;
            s.undoReady = s.redo = false;
            s.state = SState::Rec;
            break;
        }
        case SState::Rec: closeLoop(s, late, true); break;
        case SState::Play: beginDub(s); s.dubPress = true; break;
        case SState::Dub:
            if (s.dubTail >= 0) s.dubTail = -1;
            else finishDub(s);
            break;
        case SState::Stopped:
            s.pos = s.reverse ? (double) std::max<std::int64_t>(0, s.len - 1) : 0.0;
            s.state = SState::Play;
            break;
    }
}

void Helix::stopPress(Strand& s, std::int64_t late) {
    switch (s.state) {
        case SState::Rec: closeLoop(s, late, false); break;
        case SState::Play:
        case SState::Dub:
            s.state = SState::Stopped;
            s.dubTail = -1;
            s.pos = 0.0;
            break;
        default: break;
    }
}

void Helix::playPress(Strand& s, std::int64_t late) {
    if (s.state == SState::Rec) {
        closeLoop(s, late, true);
        return;
    }
    if (s.len <= 0) return;
    const auto off = late % s.len;
    s.pos = s.reverse ? (double) (s.len - 1 - off) : (double) off;
    s.state = SState::Play;
    s.dubTail = -1;
}

void Helix::recordPress(Strand& s, std::int64_t late) {
    if (s.state == SState::Empty || s.state == SState::Rec) recPress(s, late);
}

void Helix::overdubPress(Strand& s, std::int64_t late) {
    switch (s.state) {
        case SState::Empty: return;
        case SState::Rec: closeLoop(s, late, true); break;
        case SState::Dub:
            if (s.dubTail >= 0) s.dubTail = -1;
            else finishDub(s);
            return;
        case SState::Stopped:
            s.pos = s.reverse ? (double) std::max<std::int64_t>(0, s.len - 1) : 0.0;
            s.state = SState::Play;
            break;
        case SState::Play: break;
    }
    beginDub(s);
    s.dubHoldArmed = true;
}

void Helix::snapshotAhead(Strand& s, std::int64_t upTo) {
    if (s.snapDone) return;
    const std::int64_t stop = std::min(upTo, s.len);
    while (s.snapCursor < stop) {
        const auto i = (size_t) (((s.dubStart + s.dubDir * s.snapCursor) % s.len + s.len) % s.len);
        s.undo[0][i] = s.buf[0][i];
        s.undo[1][i] = s.buf[1][i];
        ++s.snapCursor;
    }
    if (s.snapCursor >= s.len) {
        s.snapDone = true;
        s.undoReady = true;
        s.undoLen = s.len;
    }
}

void Helix::undoPress(Strand& s) {
    s.rebuildAt = 0;
    if (s.state == SState::Rec) {
        clearStrand(s);
        return;
    }
    if (s.state == SState::Dub) {
        if (s.snapDone) {
            for (int c = 0; c < 2; ++c) std::swap(s.buf[(size_t) c], s.undo[(size_t) c]);
        } else {
            for (std::int64_t j = 0; j < std::min(s.dubWritten, s.len); ++j) {
                const auto i = (size_t) (((s.dubStart + s.dubDir * j) % s.len + s.len) % s.len);
                s.buf[0][i] = s.undo[0][i];
                s.buf[1][i] = s.undo[1][i];
            }
        }
        --s.layers;
        s.snapDone = true;
        s.undoReady = s.redo = false;
        leaveDub(s);
        return;
    }
    if ((s.state == SState::Play || s.state == SState::Stopped) && s.undoReady && !s.redo) {
        for (int c = 0; c < 2; ++c) std::swap(s.buf[(size_t) c], s.undo[(size_t) c]);
        std::swap(s.len, s.undoLen);
        s.pos = std::min(s.pos, (double) std::max<std::int64_t>(0, s.len - 1));
        s.rebuildAt = 0;
        s.redo = true;
        --s.layers;
    }
}

void Helix::redoPress(Strand& s) {
    s.rebuildAt = 0;
    if ((s.state == SState::Play || s.state == SState::Stopped) && s.undoReady && s.redo) {
        for (int c = 0; c < 2; ++c) std::swap(s.buf[(size_t) c], s.undo[(size_t) c]);
        std::swap(s.len, s.undoLen);
        s.pos = std::min(s.pos, (double) std::max<std::int64_t>(0, s.len - 1));
        s.redo = false;
        ++s.layers;
    }
}

void Helix::applyRev(Strand& s, bool v) {
    if (s.state == SState::Dub) leaveDub(s);
    s.reverse = v;
}

void Helix::applyHalf(Strand& s, bool v) {
    s.half = v;
    s.speed = (v ? 0.5 : 1.0) * s.tempoRatio;
}

void Helix::anchorToTransport(Strand& s, int sync, double beats, double spb) {
    if (s.len <= 0 || (s.state != SState::Play && s.state != SState::Dub)) return;
    if (sync == kSyncFollow) return;
    if (s.state == SState::Dub) leaveDub(s);
    if (sync != 0) {
        const double off = std::fmod(std::max(0.0, beats) * spb * s.speed, (double) s.len);
        s.pos = s.reverse ? (double) s.len - 1.0 - off : off;
        if (s.pos < 0.0) s.pos += (double) s.len;
    } else if (beats < 1e-6) {
        s.pos = s.reverse ? (double) (s.len - 1) : 0.0;
    }
}

Helix::Grid Helix::gridAt(int sync, double beats, double beatsPerBar) {
    const double unit = sync == 2 ? std::max(1.0, beatsPerBar) : 1.0;
    Grid g;
    g.sync = sync;
    g.inUnit = beats - std::floor(beats / unit) * unit;
    g.nextLine = (std::floor(beats / unit) + 1.0) * unit;
    g.inGrace = g.inUnit < std::min(kGraceBeats, unit * 0.5);
    return g;
}

int Helix::referenceFor(int self) const {
    for (int t = 0; t < kStrands; ++t) {
        if (t == self) continue;
        const auto& s = strands_[(size_t) t];
        if (s.len > 0 && (s.state == SState::Play || s.state == SState::Dub)) return t;
    }
    return -1;
}

Helix::Grid Helix::gridFor(int sync, int self, double beats, double beatsPerBar, double spb) const {
    if (sync != kSyncFollow) return gridAt(sync, beats, beatsPerBar);
    const int r = referenceFor(self);
    if (r < 0 || spb <= 0.0) return gridAt(kSyncBar, beats, beatsPerBar);
    const auto& ref = strands_[(size_t) r];
    const double speed = std::max(1.0e-6, std::abs(ref.speed));
    const double lap = (double) ref.len / speed / spb;
    const double since = (ref.reverse ? (double) (ref.len - 1) - ref.pos : ref.pos) / speed / spb;
    Grid g;
    g.sync = kSyncFollow;
    g.inUnit = std::max(0.0, since);
    g.nextLine = beats + std::max(0.0, lap - since);
    g.inGrace = g.inUnit < std::min(kGraceBeats, lap * 0.5);
    return g;
}

int Helix::syncOf(int t) {
    return std::clamp((int) std::lround(strandParams_[(size_t) t].sync.get(params, 2.0)), 0, kSyncFollow);
}

Helix::MasterPress Helix::masterPresses(bool playAll, bool stopAll) {
    MasterPress m;
    for (int t = 0; t < kStrands; ++t) {
        const auto& s = strands_[(size_t) t];
        const bool spent = strandParams_[(size_t) t].shot.on(params) && s.state == SState::Stopped;
        const bool running = s.state == SState::Rec || s.state == SState::Play || s.state == SState::Dub;
        m.play[(size_t) t] = playAll && s.len > 0 && s.state != SState::Rec && !spent;
        m.stop[(size_t) t] = stopAll && running;
        if (m.play[(size_t) t]) m.playSync = std::max(m.playSync, syncOf(t));
        if (m.stop[(size_t) t]) m.stopSync = std::min(m.stopSync, syncOf(t));
    }
    return m;
}

void Helix::applyPending(Strand& s) {
    const auto what = s.pending;
    const auto late = s.pendingLate;
    s.pending = Pending::None;
    s.pendingLate = 0;
    if (what == Pending::RecPress) recPress(s, late);
    else if (what == Pending::StopPress) stopPress(s, late);
    else if (what == Pending::PlayPress) playPress(s, late);
    else if (what == Pending::RecordPress) recordPress(s, late);
    else if (what == Pending::DubPress) overdubPress(s, late);
}

}
