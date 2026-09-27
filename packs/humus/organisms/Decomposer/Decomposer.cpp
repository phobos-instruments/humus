// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Decomposer/Decomposer.h"

#include <algorithm>
#include <array>
#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {
constexpr int kRefract = 3;

float chordFloor(double sensitivity) {
    return (float) (0.30 - 0.22 * sensitivity);
}

int nearestMidi(double hz) { return (int) std::lround(hum::hzToMidi(hz)); }

int velFromLevel(double level) {
    const double v = std::sqrt(std::clamp(level * 6.0, 0.0, 1.0));
    return std::clamp((int) (v * 110.0) + 17, 1, kMidiMax);
}
}

void Decomposer::prepare(double sampleRate, int maxBlock) {
    sampleRate_ = sampleRate;
    tracker_.prepare(sampleRate);
    chords_.prepare(sampleRate);
    reset();
}

void Decomposer::reset() {
    tracker_.reset();
    chords_.reset();
    heard_.fill(0);
    missed_.fill(0);
    sounding_.fill(false);
    rChordCount_.store(0);
    curNote_ = -1;
    candNote_ = -1;
    candFrames_ = 0;
    silentFrames_ = 0;
    noteHist_ = {-1, -1, -1};
    histFill_ = 0;
    slowEnv_ = 0.0;
    refractory_ = 0;
    rHz_.store(0.0f); rClar_.store(0.0f); rLevel_.store(0.0f); rNote_.store(-1);
    outCount_ = 0;
}

int Decomposer::medianNote(int raw) {
    noteHist_[2] = noteHist_[1];
    noteHist_[1] = noteHist_[0];
    noteHist_[0] = raw;
    if (histFill_ < 3) ++histFill_;
    if (histFill_ < 3) return raw;
    const int x = noteHist_[0], y = noteHist_[1], z = noteHist_[2];
    return std::max(std::min(x, y), std::min(std::max(x, y), z));
}

void Decomposer::emit(int offset, bool on, int note, int vel) {
    if (outCount_ >= (int) out_.size() || note < 0 || note > kMidiMax) return;
    MidiEvent e;
    e.data[0] = (unsigned char) ((on ? 0x90 : 0x80) | ((channel_ - 1) & 0x0f));
    e.data[1] = (unsigned char) note;
    e.data[2] = (unsigned char) (on ? vel : 0);
    e.size = 3;
    e.sampleOffset = offset;
    out_[(size_t) outCount_++] = e;
}

void Decomposer::allNotesOff(int offset) {
    if (curNote_ >= 0) { emit(offset, false, curNote_, 0); curNote_ = -1; }
    rNote_.store(-1);
}

void Decomposer::process(const float* const* in, int numIn, float* const*, int,
                         int numSamples, const Transport&) {
    channel_ = std::clamp((int) params.get("Channel", 1.0), 1, 16);
    loNote_ = std::clamp((int) params.get("LowNote", 12.0), 0, kMidiMax);
    hiNote_ = std::clamp((int) params.get("HighNote", 108.0), 0, kMidiMax);
    if (hiNote_ < loNote_ + 1) hiNote_ = loNote_ + 1;
    switch ((int) params.get("Response", 1.0)) {
        case 0: confirmFrames_ = 1; releaseFrames_ = 2; break;
        case 2: confirmFrames_ = 3; releaseFrames_ = 4; break;
        default: confirmFrames_ = 2; releaseFrames_ = 3; break;
    }
    voices_ = std::clamp((int) params.get("Voices", 1.0), 1, ChordTracker::kMaxVoices);
    const float* x = (numIn > 0 && in && in[0]) ? in[0] : nullptr;
    const int end = std::max(0, numSamples - 1);
    if (!x) return;
    if (voices_ > 1) {
        if (curNote_ >= 0) allNotesOff(end);
        chords_.setRange(loNote_, hiNote_);
        chords_.setVoices(voices_);
        chords_.setVoiceFloor(chordFloor(std::clamp(params.get("Sensitivity", 0.35), 0.0, 1.0)));
        if (chords_.push(x, numSamples) > 0) segmentChord(end);
        return;
    }
    if (rChordCount_.load() > 0) silenceChord(end);
    tracker_.setHzRange(midiToHz(loNote_) * 0.98, midiToHz(hiNote_) * 1.02);
    const int fresh = tracker_.push(x, numSamples);
    if (fresh > 0) segmentMono(end);
}

void Decomposer::silenceChord(int offset) {
    for (int note = 0; note <= kMidiMax; ++note)
        if (sounding_[(size_t) note]) { emit(offset, false, note, 0); sounding_[(size_t) note] = false; }
    heard_.fill(0);
    missed_.fill(0);
    rChordCount_.store(0);
}

void Decomposer::publishChord() {
    int at = 0;
    for (int note = 0; note <= kMidiMax && at < ChordDetectSource::kMaxChordNotes; ++note)
        if (sounding_[(size_t) note]) rChord_[(size_t) at++].store(note);
    rChordCount_.store(at);
    rNote_.store(at > 0 ? rChord_[0].load() : -1);
    rHz_.store(at > 0 ? (float) midiToHz(rChord_[0].load()) : 0.0f);
}

void Decomposer::segmentChord(int blockEndOffset) {
    const double sens = std::clamp(params.get("Sensitivity", 0.35), 0.0, 1.0);
    const double levelFloor = 0.0015 + (1.0 - sens) * 0.02;
    const double level = chords_.level();
    const bool quiet = level <= levelFloor;

    rLevel_.store((float) std::clamp(level * 4.0, 0.0, 1.0));
    rClar_.store(chords_.tonality());

    std::array<bool, kMidiMax + 1> seen{};
    if (!quiet)
        for (const auto& voice : chords_.voices())
            if (voice.note >= 0 && voice.note <= kMidiMax) seen[(size_t) voice.note] = true;

    for (int note = loNote_; note <= hiNote_; ++note) {
        const auto slot = (size_t) note;
        if (seen[slot]) {
            missed_[slot] = 0;
            if (heard_[slot] < 255) ++heard_[slot];
            if (!sounding_[slot] && heard_[slot] >= confirmFrames_) {
                sounding_[slot] = true;
                emit(blockEndOffset, true, note, chordVelocity(note));
            }
        } else {
            heard_[slot] = 0;
            if (!sounding_[slot]) continue;
            if (missed_[slot] < 255) ++missed_[slot];
            if (missed_[slot] >= releaseFrames_) {
                sounding_[slot] = false;
                emit(blockEndOffset, false, note, 0);
            }
        }
    }
    publishChord();
}

int Decomposer::chordVelocity(int note) const {
    for (const auto& voice : chords_.voices())
        if (voice.note == note) return velFromLevel(voice.salience * 0.5);
    return velFromLevel(chords_.level());
}

void Decomposer::segmentMono(int blockEndOffset) {
    const double sens = std::clamp(params.get("Sensitivity", 0.35), 0.0, 1.0);
    const double levelFloor = 0.0015 + (1.0 - sens) * 0.02;
    const double minClarity = 0.50 + (1.0 - sens) * 0.30;

    const double hz = tracker_.pitchHz();
    const double clarity = tracker_.clarity();
    const double level = tracker_.level();
    const double loHz = midiToHz(loNote_), hiHz = midiToHz(hiNote_);
    const bool voiced = hz >= loHz * 0.98 && hz <= hiHz * 1.02
                        && level > levelFloor && clarity > minClarity;

    const bool onset = level > slowEnv_ * 1.7 + levelFloor * 2.0 && refractory_ == 0;
    slowEnv_ += 0.2 * (level - slowEnv_);
    if (refractory_ > 0) --refractory_;

    rClar_.store((float) clarity);
    rLevel_.store((float) std::clamp(level * 4.0, 0.0, 1.0));
    rHz_.store(voiced ? (float) hz : 0.0f);

    if (voiced) {
        silentFrames_ = 0;
        const int raw = std::clamp(nearestMidi(hz), loNote_, hiNote_);
        const int note = medianNote(raw);
        if (note == candNote_) ++candFrames_; else { candNote_ = note; candFrames_ = 1; }

        if (curNote_ < 0) {
            if (candFrames_ >= confirmFrames_) {
                curNote_ = note;
                emit(blockEndOffset, true, curNote_, velFromLevel(level));
                rNote_.store(curNote_);
                refractory_ = kRefract;
            }
        } else if (note != curNote_ && candFrames_ >= confirmFrames_) {
            emit(blockEndOffset, false, curNote_, 0);
            curNote_ = note;
            emit(blockEndOffset, true, curNote_, velFromLevel(level));
            rNote_.store(curNote_);
            refractory_ = kRefract;
        } else if (onset && candFrames_ >= 1) {
            emit(blockEndOffset, false, curNote_, 0);
            curNote_ = note;
            emit(blockEndOffset, true, curNote_, velFromLevel(level));
            rNote_.store(curNote_);
            refractory_ = kRefract;
        }
    } else {
        candFrames_ = 0;
        histFill_ = 0;
        if (curNote_ >= 0 && ++silentFrames_ >= releaseFrames_) {
            emit(blockEndOffset, false, curNote_, 0);
            curNote_ = -1;
            rNote_.store(-1);
            rHz_.store(0.0f);
        }
    }
}

}
