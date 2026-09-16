// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Helix/Helix.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

#include "hum/dsp/SoundFileBuffer.h"

namespace hum {

std::array<Helix::StrandParams, Helix::kStrands> Helix::makeStrandParams() {
    std::array<StrandParams, kStrands> out;
    for (int t = 0; t < kStrands; ++t) {
        auto ref = [t](const char* prefix) { return ParamRef::numbered(prefix, t + 1); };
        out[(size_t) t] = {ref("Level"), ref("Mute"), ref("Solo"), ref("Sync"), ref("Rec"),
                           ref("Stop"), ref("Play"), ref("Undo"), ref("Redo"), ref("Clear"),
                           ref("Rev"), ref("Half"), ref("Shot")};
    }
    return out;
}

void Helix::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    maxLoopSamples_ = (std::int64_t) std::llround(kMaxSeconds * sampleRate);
    holdSamples_ = (std::int64_t) std::llround(kHoldSeconds * sampleRate);
    for (auto& s : strands_) {
        for (auto& ch : s.buf) ch.assign((size_t) maxLoopSamples_, 0.0f);
        for (auto& ch : s.undo) ch.assign((size_t) maxLoopSamples_, 0.0f);
    }
    reset();
    loadSessionAudio();
}

void Helix::reset() {
    for (auto& s : strands_) {
        clearStrand(s);
        s.prevRec = s.prevStop = s.prevUndo = s.prevClear = false;
        s.prevRedo = false;
        s.dubPress = false;
        s.heldSamples = 0;
        s.level = s.levelPrev = 1.0f;
        s.half = false;
        s.speed = 1.0;
        s.revPend = s.halfPend = -1;
    }
    prevRolling_ = false;
    expectBeats_ = 0.0;
}

void Helix::clearStrand(Strand& s) {
    s.len = s.recCount = 0;
    s.pos = 0.0;
    s.state = SState::Empty;
    s.dubStart = s.dubWritten = s.snapCursor = 0;
    s.lastWrite = -1;
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
    s.layers = 1;
    s.undoReady = s.redo = false;
    s.snapDone = true;
    s.state = thenPlay ? SState::Play : SState::Stopped;
}

void Helix::beginDub(Strand& s) {
    s.dubStart = (std::int64_t) s.pos;
    s.dubDir = s.reverse ? -1 : 1;
    s.dubWritten = 0;
    s.lastWrite = -1;
    s.snapCursor = 0;
    s.snapDone = s.len <= 0;
    s.undoReady = s.redo = false;
    ++s.layers;
    s.state = SState::Dub;
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
        case SState::Dub: s.state = SState::Play; break;
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
            s.pos = 0.0;
            break;
        default: break;
    }
}

void Helix::playPress(Strand& s, std::int64_t late) {
    if (s.len <= 0 || s.state == SState::Rec) return;
    const auto off = late % s.len;
    s.pos = s.reverse ? (double) (s.len - 1 - off) : (double) off;
    s.state = SState::Play;
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
    }
}

void Helix::undoPress(Strand& s) {
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
        s.state = SState::Play;
        return;
    }
    if ((s.state == SState::Play || s.state == SState::Stopped) && s.undoReady && !s.redo) {
        for (int c = 0; c < 2; ++c) std::swap(s.buf[(size_t) c], s.undo[(size_t) c]);
        s.redo = true;
        --s.layers;
    }
}

void Helix::redoPress(Strand& s) {
    if ((s.state == SState::Play || s.state == SState::Stopped) && s.undoReady && s.redo) {
        for (int c = 0; c < 2; ++c) std::swap(s.buf[(size_t) c], s.undo[(size_t) c]);
        s.redo = false;
        ++s.layers;
    }
}

void Helix::applyRev(Strand& s, bool v) {
    if (s.state == SState::Dub) s.state = SState::Play;
    s.reverse = v;
}

void Helix::applyHalf(Strand& s, bool v) {
    s.half = v;
    s.speed = v ? 0.5 : 1.0;
}

void Helix::anchorToTransport(Strand& s, int sync, double beats, double spb) {
    if (s.len <= 0 || (s.state != SState::Play && s.state != SState::Dub)) return;
    if (s.state == SState::Dub) s.state = SState::Play;
    if (sync != 0) {
        const double off = std::fmod(std::max(0.0, beats) * spb * s.speed, (double) s.len);
        s.pos = s.reverse ? (double) s.len - 1.0 - off : off;
        if (s.pos < 0.0) s.pos += (double) s.len;
    } else if (beats < 1e-6) {
        s.pos = s.reverse ? (double) (s.len - 1) : 0.0;
    }
}

void Helix::applyPending(Strand& s) {
    const auto what = s.pending;
    const auto late = s.pendingLate;
    s.pending = Pending::None;
    s.pendingLate = 0;
    if (what == Pending::RecPress) recPress(s, late);
    else if (what == Pending::StopPress) stopPress(s, late);
    else if (what == Pending::PlayPress) playPress(s, late);
}

void Helix::process(const float* const* in, int numIn, float* const* out, int numOut,
                    int numSamples, const Transport& transport) {
    for (int c = 0; c < numOut; ++c) std::fill(out[c], out[c] + numSamples, 0.0f);
    if (numSamples <= 0) return;

    const bool monitor = params.get("Monitor", 1.0) >= 0.5;
    decay_ = std::clamp(params.get("Decay", 1.0), 0.0, 1.0);
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

    std::array<int, kStrands> pendAt {}, revAt {}, halfAt {};
    for (int t = 0; t < kStrands; ++t) {
        auto& s = strands_[(size_t) t];
        const auto& sp = strandParams_[(size_t) t];
        s.level = (float) std::clamp(sp.level.get(params, 1.0), 0.0, 1.0);
        const bool audible = !sp.mute.on(params) && (!anySolo || sp.solo.on(params));
        if (!audible) s.level = 0.0f;
        const int sync = std::clamp((int) std::lround(sp.sync.get(params, 2.0)), 0, 2);
        const bool rec = sp.rec.on(params);
        const bool stop = sp.stop.on(params);
        const bool play = sp.play.on(params);
        const bool undo = sp.undo.on(params);
        const bool redo = sp.redo.on(params);
        const bool clear = sp.clear.on(params);
        const bool rev = sp.rev.on(params);
        const bool half = sp.half.on(params);
        s.oneShot = sp.shot.on(params);

        if (jumped) anchorToTransport(s, sync, beats0, spb);

        const double unit = sync == 2 ? std::max(1.0, transport.beatsPerBar()) : 1.0;
        const double inUnit = beats0 - std::floor(beats0 / unit) * unit;
        const double nextLine = (std::floor(beats0 / unit) + 1.0) * unit;
        const bool inGrace = inUnit < std::min(kGraceBeats, unit * 0.5);
        const bool quant = sync != 0 && rolling && s.len > 0
                           && (s.state == SState::Play || s.state == SState::Dub);

        auto latch = [&](bool want, bool have, int& pend, double& beat, auto&& apply) {
            if (want == have) { pend = -1; return; }
            if (pend == (want ? 1 : 0)) return;
            if (!quant || inGrace) { apply(s, want); pend = -1; return; }
            pend = want ? 1 : 0;
            beat = nextLine;
        };
        latch(rev, s.reverse, s.revPend, s.revBeat,
              [this](Strand& x, bool v) { applyRev(x, v); });
        latch(half, s.half, s.halfPend, s.halfBeat,
              [this](Strand& x, bool v) { applyHalf(x, v); });

        if (clear && !s.prevClear) clearStrand(s);
        if (undo && !s.prevUndo) undoPress(s);
        if (redo && !s.prevRedo) redoPress(s);

        auto press = [&](Pending what) {
            s.pending = what;
            s.pendingBeat = beats0;
            s.pendingLate = 0;
            if (sync == 0 || !rolling) return;
            if (inGrace)
                s.pendingLate = (std::int64_t) std::llround(inUnit * spb);
            else
                s.pendingBeat = nextLine;
        };
        if (rec && !s.prevRec) {
            s.heldSamples = 0;
            press(Pending::RecPress);
        }
        if (stop && !s.prevStop) press(Pending::StopPress);
        if (play && !s.prevPlay) press(Pending::PlayPress);
        if (!rec && s.prevRec) {
            if (s.dubPress && s.state == SState::Dub && s.heldSamples >= holdSamples_)
                s.state = SState::Play;
            s.dubPress = false;
        }
        if (rec) s.heldSamples += numSamples;
        s.prevRec = rec;
        s.prevStop = stop;
        s.prevPlay = play;
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
            switch (s.state) {
                case SState::Rec:
                    if (s.recCount < maxLoopSamples_) {
                        s.buf[0][(size_t) s.recCount] = inL;
                        s.buf[1][(size_t) s.recCount] = inR;
                        if (++s.recCount >= maxLoopSamples_) closeLoop(s, 0, true);
                    }
                    break;
                case SState::Play:
                case SState::Dub: {
                    if (frozen) break;
                    const auto i0 = (std::int64_t) s.pos;
                    const auto i1 = i0 + 1 >= s.len ? 0 : i0 + 1;
                    const float fr = (float) (s.pos - (double) i0);
                    const float l = s.buf[0][(size_t) i0] * (1.0f - fr)
                                    + s.buf[0][(size_t) i1] * fr;
                    const float r = s.buf[1][(size_t) i0] * (1.0f - fr)
                                    + s.buf[1][(size_t) i1] * fr;
                    if (s.state == SState::Dub && i0 != s.lastWrite) {
                        snapshotAhead(s, s.dubWritten + 1);
                        s.buf[0][(size_t) i0] = (float) (s.buf[0][(size_t) i0] * decay_) + inL;
                        s.buf[1][(size_t) i0] = (float) (s.buf[1][(size_t) i0] * decay_) + inR;
                        s.lastWrite = i0;
                        ++s.dubWritten;
                    }
                    const float lvl = s.levelPrev
                                      + (s.level - s.levelPrev) * (float) n / (float) numSamples;
                    const float dl = l * lvl, dr = r * lvl;
                    oL += dl;
                    oR += dr;
                    const int d0 = 2 + t * 2;
                    if (numOut > d0) out[d0][n] = dl;
                    if (numOut > d0 + 1) out[d0 + 1][n] = dr;
                    bool wrapped = false;
                    s.pos += s.reverse ? -s.speed : s.speed;
                    if (s.pos >= (double) s.len) { s.pos -= (double) s.len; wrapped = true; }
                    else if (s.pos < 0.0) { s.pos += (double) s.len; wrapped = true; }
                    if (wrapped && s.oneShot && s.state == SState::Play) {
                        s.state = SState::Stopped;
                        s.pos = 0.0;
                    }
                    break;
                }
                default: break;
            }
        }
        if (numOut > 0) out[0][n] = oL;
        if (numOut > 1) out[1][n] = oR;
    }

    for (auto& s : strands_) {
        s.levelPrev = s.level;
        s.uiState.store((int) s.state, std::memory_order_relaxed);
        s.uiPhase.store(s.len > 0 && (s.state == SState::Play || s.state == SState::Dub)
                            ? (float) ((double) s.pos / (double) s.len)
                            : -1.0f,
                        std::memory_order_relaxed);
        s.uiLayers.store(s.layers, std::memory_order_relaxed);
        s.uiPending.store(s.pending != Pending::None || s.revPend >= 0 || s.halfPend >= 0,
                          std::memory_order_relaxed);
    }
}

bool Helix::storeSessionAudio(const std::string& pathPrefix,
                              std::vector<std::pair<std::string, std::string>>& out) {
    bool any = false;
    for (int t = 0; t < kStrands; ++t) {
        auto& s = strands_[(size_t) t];
        const std::string param = "Loop" + std::to_string(t + 1);
        if (s.len <= 0) {
            if (!params.getText(param).empty()) {
                out.push_back({param, {}});
                any = true;
            }
            continue;
        }
        juce::AudioBuffer<float> b(2, (int) s.len);
        for (int c = 0; c < 2; ++c)
            b.copyFrom(c, 0, s.buf[(size_t) c].data(), (int) s.len);
        const std::string path = pathPrefix + "-loop" + std::to_string(t + 1) + ".wav";
        if (!writeSoundFile(path, b, sampleRate_)) continue;
        out.push_back({param, path});
        strands_[(size_t) t].loadedUri = path;
        any = true;
    }
    return any;
}

void Helix::loadSessionAudio() {
    for (int t = 0; t < kStrands; ++t) {
        auto& s = strands_[(size_t) t];
        if (maxLoopSamples_ <= 0) continue;
        const auto uri = params.getText("Loop" + std::to_string(t + 1));
        if (uri.empty() || (uri == s.loadedUri && s.state != SState::Empty)) continue;
        if (s.state == SState::Rec || s.state == SState::Dub) continue;
        if (s.state != SState::Empty) clearStrand(s);
        juce::AudioBuffer<float> b;
        double fsr = 0.0;
        if (!loadSoundFile(uri, b, fsr) || b.getNumSamples() <= 0) continue;
        const double ratio = fsr > 0.0 ? sampleRate_ / fsr : 1.0;
        const auto len = std::min<std::int64_t>(
            maxLoopSamples_, (std::int64_t) std::llround((double) b.getNumSamples() * ratio));
        if (len <= 0) continue;
        const int src = b.getNumSamples();
        for (int c = 0; c < 2; ++c) {
            const float* from = b.getReadPointer(std::min(c, b.getNumChannels() - 1));
            for (std::int64_t i = 0; i < len; ++i) {
                const double x = (double) i / ratio;
                const int a = std::min((int) x, src - 1);
                const int a1 = std::min(a + 1, src - 1);
                const float fr = (float) (x - (double) a);
                s.buf[(size_t) c][(size_t) i] = from[a] * (1.0f - fr) + from[a1] * fr;
            }
        }
        s.pos = 0.0;
        s.len = len;
        s.layers = 1;
        s.loadedUri = uri;
        s.state = SState::Stopped;
        s.uiState.store((int) SState::Stopped, std::memory_order_relaxed);
        s.uiLayers.store(1, std::memory_order_relaxed);
    }
}

int Helix::strandState(int strand) const {
    return strand >= 0 && strand < kStrands
               ? strands_[(size_t) strand].uiState.load(std::memory_order_relaxed) : 0;
}

float Helix::strandPhase(int strand) const {
    return strand >= 0 && strand < kStrands
               ? strands_[(size_t) strand].uiPhase.load(std::memory_order_relaxed) : -1.0f;
}

int Helix::strandLayers(int strand) const {
    return strand >= 0 && strand < kStrands
               ? strands_[(size_t) strand].uiLayers.load(std::memory_order_relaxed) : 0;
}

bool Helix::strandPending(int strand) const {
    return strand >= 0 && strand < kStrands
           && strands_[(size_t) strand].uiPending.load(std::memory_order_relaxed);
}

}
