// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Helix/Helix.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace hum {

std::array<Helix::StrandParams, Helix::kStrands> Helix::makeStrandParams() {
    std::array<StrandParams, kStrands> out;
    for (int t = 0; t < kStrands; ++t) {
        auto ref = [t](const char* prefix) { return ParamRef::numbered(prefix, t + 1); };
        out[(size_t) t] = {ref("Level"), ref("Mute"), ref("Solo"), ref("Sync"), ref("Rec"),
                           ref("Stop"), ref("Play"), ref("Undo"), ref("Redo"), ref("Clear"),
                           ref("Rev"), ref("Half"), ref("Shot"), ref("Tempo"), ref("Record"), ref("Dub"), ref("Monitor"),
                           ref("Slice"), ref("Nudge")};
    }
    return out;
}

void Helix::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    maxLoopSamples_ = (std::int64_t) std::llround(kMaxSeconds * sampleRate);
    holdSamples_ = (std::int64_t) std::llround(kHoldSeconds * sampleRate);
    for (size_t t = 0; t < kStrands; ++t) {
        for (auto& ch : strands_[t].buf) ch.assign((size_t) maxLoopSamples_, 0.0f);
        for (auto& ch : strands_[t].undo) ch.assign((size_t) maxLoopSamples_, 0.0f);
        const auto chunks = (size_t) (maxLoopSamples_ / kPeakChunk + 2);
        strands_[t].chunkPeak.assign(chunks, 0.0f);
        strands_[t].undoChunkPeak.assign(chunks, 0.0f);
        stretch_[t].prepare(sampleRate);
    }
    dubLag_ = stretch_[0].dubLatency();
    reset();
    loadSessionAudio();
}

void Helix::reset() {
    for (size_t t = 0; t < kStrands; ++t) {
        auto& s = strands_[t];
        clearStrand(s);
        s.prevRec = s.prevStop = s.prevUndo = s.prevClear = false;
        s.prevRedo = false;
        s.dubPress = false;
        s.heldSamples = 0;
        s.prevRecord = s.prevDub = s.dubHoldArmed = false;
        s.dubHeldSamples = 0;
        s.level = s.levelPrev = 1.0f;
        s.half = false;
        s.speed = 1.0;
        s.tempoRatio = 1.0;
        s.stretch = false;
        s.revPend = s.halfPend = -1;
        stretch_[t].reset();
    }
    prevRolling_ = false;
    prevPlayAll_ = prevStopAll_ = false;
    expectBeats_ = 0.0;
}

void Helix::followTempo(Strand& s, int mode, bool canStretch) const {
    if (s.len > 0 && s.recBpm <= 0.0) s.recBpm = tempoNow_;
    s.tempoRatio = mode == kTempoOff || s.recBpm <= 0.0
                       ? 1.0
                       : std::clamp(tempoNow_ / s.recBpm, kMinRatio, kMaxRatio);
    s.stretch = mode == kTempoStretch && canStretch;
    s.speed = (s.half ? 0.5 : 1.0) * s.tempoRatio;
}

void Helix::process(const float* const* in, int numIn, float* const* out, int numOut,
                    int numSamples, const Transport& transport) {
    for (int c = 0; c < numOut; ++c) std::fill(out[c], out[c] + numSamples, 0.0f);
    if (numSamples <= 0) return;

    const double monitorAll = params.get("Monitor", kMonitorMix);
    std::array<int, kStrands> monitorOf {};
    bool monitor = false;
    for (int t = 0; t < kStrands; ++t) {
        monitorOf[(size_t) t] =
            std::clamp((int) std::lround(strandParams_[(size_t) t].monitor.get(params, monitorAll)), 0, 2);
        monitor = monitor || monitorOf[(size_t) t] == kMonitorMix;
    }
    decay_ = std::clamp(params.get("Decay", 1.0), 0.0, 1.0);
    tempoNow_ = transport.tempo();
    barBeats_ = transport.beatsPerBar();
    const double spb = transport.samplesPerBeat();
    const double beats0 = transport.beats();
    const bool rolling = transport.playing() && spb > 0.0;
    const bool frozen = params.get("Follow", 1.0) >= 0.5 && !transport.playing();

    bool anySolo = false;
    for (int t = 0; t < kStrands; ++t)
        anySolo = anySolo || strandParams_[(size_t) t].solo.on(params);

    const bool jumped = rolling
                        && (!prevRolling_ || std::abs(beats0 - expectBeats_) > 0.25);
    prevRolling_ = rolling;
    expectBeats_ = rolling ? beats0 + (double) numSamples / spb : beats0;

    const bool playAll = params.get("PlayAll", 0.0) >= 0.5;
    const bool stopAll = params.get("StopAll", 0.0) >= 0.5;
    const auto master = masterPresses(playAll && !prevPlayAll_, stopAll && !prevStopAll_);
    prevPlayAll_ = playAll;
    prevStopAll_ = stopAll;
    const double bar = transport.beatsPerBar();
    const Grid playAllGrid = gridFor(master.playSync, -1, beats0, bar, spb);
    const Grid stopAllGrid = gridFor(master.stopSync, -1, beats0, bar, spb);

    std::array<int, kStrands> pendAt {}, revAt {}, halfAt {};
    for (int t = 0; t < kStrands; ++t) {
        auto& s = strands_[(size_t) t];
        const auto& sp = strandParams_[(size_t) t];
        s.level = (float) std::clamp(sp.level.get(params, 1.0), 0.0, 1.0);
        s.nudge = s.state == SState::Play ? (int) std::lround(std::clamp(sp.nudge.get(params, 0.0), -1.0, 1.0)) : 0;
        s.slice = (int) std::lround(std::clamp(sp.slice.get(params, 0.0), 0.0, (double) kSlices));
        if (s.slice != s.slicePrev) {
            if (s.slice > 0 && s.len > 0)
                s.pos = (double) ((s.slice - 1) * (s.len / kSlices));
            s.slicePrev = s.slice;
        }
        const bool audible = !sp.mute.on(params) && (!anySolo || sp.solo.on(params));
        if (!audible) s.level = 0.0f;
        const int sync = syncOf(t);
        const bool rec = sp.rec.on(params);
        const bool stop = sp.stop.on(params);
        const bool play = sp.play.on(params);
        const bool record = sp.record.on(params);
        const bool dub = sp.dub.on(params);
        const bool undo = sp.undo.on(params);
        const bool redo = sp.redo.on(params);
        const bool clear = sp.clear.on(params);
        const bool rev = sp.rev.on(params);
        const bool half = sp.half.on(params);
        s.oneShot = sp.shot.on(params);
        followTempo(s, std::clamp((int) std::lround(sp.tempo.get(params, kTempoPitch)), 0, 2),
                    stretch_[(size_t) t].available());

        if (jumped) anchorToTransport(s, sync, beats0, spb);

        const Grid grid = gridFor(sync, t, beats0, bar, spb);
        const bool quant = sync != 0 && rolling && s.len > 0
                           && (s.state == SState::Play || s.state == SState::Dub);

        auto latch = [&](bool want, bool have, int& pend, double& beat, auto&& apply) {
            if (want == have) { pend = -1; return; }
            if (pend == (want ? 1 : 0)) return;
            if (!quant || grid.inGrace) { apply(s, want); pend = -1; return; }
            pend = want ? 1 : 0;
            beat = grid.nextLine;
        };
        latch(rev, s.reverse, s.revPend, s.revBeat,
              [this](Strand& x, bool v) { applyRev(x, v); });
        latch(half, s.half, s.halfPend, s.halfBeat,
              [this](Strand& x, bool v) { applyHalf(x, v); });

        if (clear && !s.prevClear) clearStrand(s);
        if (undo && !s.prevUndo) undoPress(s);
        if (redo && !s.prevRedo) redoPress(s);

        auto press = [&](Pending what, const Grid& at) {
            s.pending = what;
            s.pendingBeat = beats0;
            s.pendingLate = 0;
            if (at.sync == 0 || !rolling) return;
            if (at.inGrace)
                s.pendingLate = (std::int64_t) std::llround(at.inUnit * spb);
            else
                s.pendingBeat = at.nextLine;
        };
        if (rec && !s.prevRec) {
            s.heldSamples = 0;
            press(Pending::RecPress, grid);
        }
        if (stop && !s.prevStop) press(Pending::StopPress, grid);
        if (play && !s.prevPlay) press(Pending::PlayPress, grid);
        if (record && !s.prevRecord) press(Pending::RecordPress, grid);
        if (dub && !s.prevDub) {
            s.dubHeldSamples = 0;
            press(Pending::DubPress, grid);
        }
        if (master.play[(size_t) t]) press(Pending::PlayPress, playAllGrid);
        if (master.stop[(size_t) t]) press(Pending::StopPress, stopAllGrid);
        if (!rec && s.prevRec) {
            if (s.dubPress && s.state == SState::Dub && s.heldSamples >= holdSamples_)
                finishDub(s);
            s.dubPress = false;
        }
        if (rec) s.heldSamples += numSamples;
        if (!dub && s.prevDub) {
            if (s.dubHoldArmed && s.state == SState::Dub && s.dubHeldSamples >= holdSamples_) finishDub(s);
            s.dubHoldArmed = false;
        }
        if (dub) s.dubHeldSamples += numSamples;
        s.prevRec = rec;
        s.prevStop = stop;
        s.prevPlay = play;
        s.prevRecord = record;
        s.prevDub = dub;
        s.prevUndo = undo;
        s.prevRedo = redo;
        s.prevClear = clear;

        auto sampleAt = [&](double beat) {
            std::int64_t off = 0;
            if (rolling) off = (std::int64_t) std::llround((beat - beats0) * spb);
            if (off < 0) off = 0;
            return off < numSamples ? (int) off : -1;
        };
        pendAt[(size_t) t] = s.pending != Pending::None ? sampleAt(s.pendingBeat) : -1;
        revAt[(size_t) t] = s.revPend >= 0 ? sampleAt(s.revBeat) : -1;
        halfAt[(size_t) t] = s.halfPend >= 0 ? sampleAt(s.halfBeat) : -1;

        snapshotAhead(s, s.snapCursor + (std::int64_t) kSnapSpeed * numSamples);
    }

    for (int n = 0; n < numSamples; ++n) {
        const float inL = (numIn > 0 && in && in[0]) ? in[0][n] : 0.0f;
        const float inR = (numIn > 1 && in && in[1]) ? in[1][n] : inL;
        float oL = monitor ? inL : 0.0f;
        float oR = monitor ? inR : 0.0f;

        for (int t = 0; t < kStrands; ++t) {
            auto& s = strands_[(size_t) t];
            if (pendAt[(size_t) t] == n) applyPending(s);
            if (revAt[(size_t) t] == n && s.revPend >= 0) {
                applyRev(s, s.revPend > 0);
                s.revPend = -1;
            }
            if (halfAt[(size_t) t] == n && s.halfPend >= 0) {
                applyHalf(s, s.halfPend > 0);
                s.halfPend = -1;
            }
            const int own = 2 + 2 * t;
            const float ownL = (numIn > own && in && in[own]) ? in[own][n] : 0.0f;
            const float ownR = (numIn > own + 1 && in && in[own + 1]) ? in[own + 1][n] : ownL;
            if (monitorOf[(size_t) t] == kMonitorMix) {
                oL += ownL;
                oR += ownR;
            }
            s.inPeak = std::max(s.inPeak, std::max(std::fabs(inL + ownL), std::fabs(inR + ownR)));
            runStrand(t, inL + ownL, inR + ownR, n, numSamples, frozen, out, numOut, oL, oR);
            if (monitorOf[(size_t) t] == kMonitorOut && numOut > own + 1) {
                out[own][n] += inL + ownL;
                out[own + 1][n] += inR + ownR;
            }
        }
        if (numOut > 0) out[0][n] = oL;
        if (numOut > 1) out[1][n] = oR;
    }

    for (auto& s : strands_) s.levelPrev = s.level;
    publishUi();
}

}
