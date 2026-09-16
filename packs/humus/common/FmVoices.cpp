// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "common/FmVoices.h"

#include <algorithm>

#include "hum/dsp/DspMath.h"

namespace hum {

void FmVoices::prepare(double hostRate) {
    hostRate_ = hostRate > 0.0 ? hostRate : kDefaultSampleRate;
    reset();
}

void FmVoices::setChipCount(int chips) {
    const int want = std::clamp(chips, 1, kMaxChips);
    if (want == chips_) return;
    chips_ = want;
    reset();
}

void FmVoices::reset() {
    bendSemis_.fill(0.0);
    for (auto& c : chip_) c.allOff();
    slots_.fill({});
    program_.fill(0);
    ring_.reset();
    clock_ = 0;
}

void FmVoices::programChange(int channel, int program) {
    if (!valid(channel)) return;
    program_[(size_t) channel] = std::clamp(program, 0, GmBank::kPrograms - 1);
}

int FmVoices::programOf(int channel) const {
    return valid(channel) ? program_[(size_t) channel] : 0;
}

int FmVoices::findFree() const {
    for (int v = 0; v < voiceCapacity(); ++v)
        if (slots_[(size_t) v].channel < 0) return v;
    return -1;
}

int FmVoices::findOldest() const {
    int best = -1;
    uint64_t oldest = 0;
    for (int v = 0; v < voiceCapacity(); ++v) {
        const auto& s = slots_[(size_t) v];
        if (s.channel < 0) continue;
        if (best < 0 || s.stamp < oldest) { best = v; oldest = s.stamp; }
    }
    return best;
}

void FmVoices::stop(int voice) {
    if (voice < 0 || voice >= voiceCapacity()) return;
    chip_[(size_t) (voice / FmChip::kChannels)].voiceOff(voice % FmChip::kChannels);
    slots_[(size_t) voice] = {};
}

void FmVoices::noteOn(int channel, int note, int velocity) {
    if (!valid(channel) || bank_ == nullptr || !bank_->ready()) return;
    if (velocity <= 0) { noteOff(channel, note); return; }
    const bool drum = channel == kDrumChannel;
    const auto& entry = drum ? bank_->percussion(note)
                             : bank_->melodic(program_[(size_t) channel]);
    const int keyed = drum && entry.percussionKey > 0 ? entry.percussionKey : note;
    const double hz = midiToHz((double) (keyed + entry.noteOffset));

    int voice = findFree();
    if (voice < 0) voice = findOldest();
    if (voice < 0) return;
    stop(voice);
    slots_[(size_t) voice] = {channel, note, ++clock_};
    chip_[(size_t) (voice / FmChip::kChannels)]
        .retune(voice % FmChip::kChannels, bendSemis_[(size_t) channel]);
    chip_[(size_t) (voice / FmChip::kChannels)]
        .voiceOn(voice % FmChip::kChannels, note, velocity, hz, entry.patch);
}

void FmVoices::pitchBend(int channel, double semitones) {
    if (!valid(channel)) return;
    bendSemis_[(size_t) channel] = semitones;
    for (int v = 0; v < voiceCapacity(); ++v)
        if (slots_[(size_t) v].channel == channel)
            chip_[(size_t) (v / FmChip::kChannels)].retune(v % FmChip::kChannels, semitones);
}

void FmVoices::noteOff(int channel, int note) {
    if (!valid(channel)) return;
    for (int v = 0; v < voiceCapacity(); ++v) {
        const auto& s = slots_[(size_t) v];
        if (s.channel == channel && s.note == note) stop(v);
    }
}

void FmVoices::allNotesOff(int channel) {
    for (int v = 0; v < voiceCapacity(); ++v)
        if (slots_[(size_t) v].channel == channel || channel < 0) stop(v);
}

int FmVoices::sounding() const {
    int n = 0;
    for (int v = 0; v < voiceCapacity(); ++v)
        if (slots_[(size_t) v].channel >= 0) ++n;
    return n;
}

void FmVoices::render(float* left, float* right, int numSamples, float level) {
    const double ratio = FmChip::kRate / hostRate_;
    constexpr int kChunk = RateRing::kChunk;
    while (ring_.needsMore(numSamples, ratio)) {
        float sumL[kChunk] = {}, sumR[kChunk] = {};
        float chipL[kChunk], chipR[kChunk];
        for (int c = 0; c < chips_; ++c) {
            chip_[(size_t) c].render(chipL, chipR, kChunk);
            for (int i = 0; i < kChunk; ++i) {
                sumL[i] += chipL[i];
                sumR[i] += chipR[i];
            }
        }
        for (int i = 0; i < kChunk; ++i) ring_.push(sumL[i], sumR[i]);
    }
    ring_.read(left, right, numSamples, ratio, level);
}

}
