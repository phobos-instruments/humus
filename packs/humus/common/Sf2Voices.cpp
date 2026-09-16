// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "common/Sf2Voices.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {

constexpr double kShortestStage = 0.001;

float stageRate(double timecents, double hostRate) {
    const double seconds = std::max(kShortestStage, sf2::timecentsToSeconds(timecents));
    return (float) (1.0 / (seconds * hostRate));
}

}

bool Sf2Voices::load(const juce::File& file) {
    reset();
    data_ = sf2::loadFile(file);
    return ready();
}

void Sf2Voices::prepare(double hostRate) {
    hostRate_ = hostRate > 0.0 ? hostRate : kDefaultSampleRate;
    reset();
}

void Sf2Voices::reset() {
    bendSemis_.fill(0.0);
    voices_.fill({});
    program_.fill(0);
    clock_ = 0;
}

void Sf2Voices::programChange(int channel, int program) {
    if (!valid(channel)) return;
    program_[(size_t) channel] = std::clamp(program, 0, kMidiMax);
}

int Sf2Voices::programOf(int channel) const {
    return valid(channel) ? program_[(size_t) channel] : 0;
}

const sf2::Preset* Sf2Voices::presetFor(int channel) const {
    if (!ready() || !valid(channel)) return nullptr;
    const int wanted = program_[(size_t) channel];
    const int bank = channel == kDrumChannel ? kDrumBank : 0;
    const sf2::Preset* sameProgram = nullptr;
    const sf2::Preset* sameBank = nullptr;
    for (const auto& p : data_.presets) {
        if (p.bank == bank && p.program == wanted) return &p;
        if (p.bank == bank && sameBank == nullptr) sameBank = &p;
        if (p.program == wanted && sameProgram == nullptr) sameProgram = &p;
    }
    if (bank == kDrumBank && sameBank != nullptr) return sameBank;
    if (sameProgram != nullptr) return sameProgram;
    return &data_.presets.front();
}

std::string Sf2Voices::programName(int program) const {
    if (!ready()) return {};
    for (const auto& p : data_.presets)
        if (p.bank == 0 && p.program == program) return p.name;
    return {};
}

int Sf2Voices::take() {
    for (int v = 0; v < kVoices; ++v)
        if (voices_[(size_t) v].zone == nullptr) return v;
    int best = -1;
    std::uint64_t oldest = 0;
    for (int v = 0; v < kVoices; ++v) {
        const auto& s = voices_[(size_t) v];
        if (best < 0 || s.stamp < oldest) { best = v; oldest = s.stamp; }
    }
    return best;
}

void Sf2Voices::start(const sf2::Zone& z, int channel, int note, int velocity) {
    if (z.sampleIndex < 0 || (size_t) z.sampleIndex >= data_.samples.size()) return;
    const auto& smp = data_.samples[(size_t) z.sampleIndex];
    const int voice = take();
    if (voice < 0) return;

    Voice v;
    v.zone = &z;
    v.channel = channel;
    v.note = note;
    v.stamp = ++clock_;
    v.held = true;

    v.start = smp.start + (std::uint32_t) std::max(0, (int) z.startOffset);
    v.end = smp.end + (std::uint32_t) std::min(0, (int) z.endOffset);
    v.loopStart = smp.loopStart + (std::uint32_t) z.loopStartOffset;
    v.loopEnd = smp.loopEnd + (std::uint32_t) z.loopEndOffset;
    if (v.end > data_.pcm.size()) v.end = (std::uint32_t) data_.pcm.size();
    if (v.loopEnd <= v.loopStart || v.loopEnd > v.end) v.looping = false;
    else v.looping = (z.sampleModes & 1) != 0;

    const int root = z.rootKey >= 0 ? z.rootKey : smp.originalKey;
    const double cents = (double) (note - root) * (double) z.scaleTuning
                       + (double) z.coarseTune * 100.0 + (double) z.fineTune
                       + (double) smp.correction;
    v.step = (double) smp.sampleRate / hostRate_ * std::pow(2.0, cents / 1200.0);
    v.pos = (double) v.start;
    v.bend = std::pow(2.0, bendSemis_[(size_t) channel] / 12.0);

    const double vel = (double) std::clamp(velocity, 1, kMidiMax) / (double) kMidiMax;
    const float gain = (float) (sf2::centibelsToGain(z.attenuationCb) * vel * vel);
    const double pan = std::clamp(z.panTenthPct / 1000.0, -0.5, 0.5);
    v.peakL = gain * (float) std::cos((pan + 0.5) * juce::MathConstants<double>::halfPi);
    v.peakR = gain * (float) std::sin((pan + 0.5) * juce::MathConstants<double>::halfPi);

    v.attack = stageRate(z.attackTc, hostRate_);
    v.decay = stageRate(z.decayTc, hostRate_);
    v.release = stageRate(z.releaseTc, hostRate_);
    v.sustain = (float) std::clamp(sf2::centibelsToGain(z.sustainCb), 0.0, 1.0);
    v.env = 0.0f;

    voices_[(size_t) voice] = v;
}

void Sf2Voices::noteOn(int channel, int note, int velocity) {
    if (!valid(channel) || !ready()) return;
    if (velocity <= 0) { noteOff(channel, note); return; }
    const auto* preset = presetFor(channel);
    if (preset == nullptr) return;
    int layers = 0;
    for (const auto& z : preset->zones) {
        if (note < z.keyLo || note > z.keyHi) continue;
        if (velocity < z.velLo || velocity > z.velHi) continue;
        start(z, channel, note, velocity);
        if (++layers >= kLayers) break;
    }
}

void Sf2Voices::pitchBend(int channel, double semitones) {
    if (!valid(channel)) return;
    bendSemis_[(size_t) channel] = semitones;
    const double ratio = std::pow(2.0, semitones / 12.0);
    for (auto& v : voices_)
        if (v.zone != nullptr && v.channel == channel) v.bend = ratio;
}

void Sf2Voices::noteOff(int channel, int note) {
    for (auto& v : voices_)
        if (v.zone != nullptr && v.channel == channel && v.note == note) v.held = false;
}

void Sf2Voices::allNotesOff(int channel) {
    for (auto& v : voices_)
        if (v.zone != nullptr && (channel < 0 || v.channel == channel)) v.held = false;
}

int Sf2Voices::sounding() const {
    int n = 0;
    for (const auto& v : voices_)
        if (v.zone != nullptr) ++n;
    return n;
}

float Sf2Voices::sampleAt(const Voice& v, double pos) const {
    const auto i = (std::uint32_t) pos;
    if (i + 1 >= data_.pcm.size()) return 0.0f;
    const float a = (float) data_.pcm[i] / 32768.0f;
    const float b = (float) data_.pcm[i + 1] / 32768.0f;
    return a + (b - a) * (float) (pos - (double) i);
}

void Sf2Voices::render(float* left, float* right, int numSamples, float level) {
    const bool split = left != right;
    for (int i = 0; i < numSamples; ++i) {
        left[i] = 0.0f;
        if (split) right[i] = 0.0f;
    }
    if (!ready()) return;

    for (auto& v : voices_) {
        if (v.zone == nullptr) continue;
        for (int i = 0; i < numSamples; ++i) {
            if (v.held) {
                if (v.env < 1.0f && v.attack < 1.0f) v.env = std::min(1.0f, v.env + v.attack);
                else if (v.env > v.sustain) v.env = std::max(v.sustain, v.env - v.decay);
                else v.env = std::max(v.env, v.sustain);
            } else {
                v.env -= v.release;
            }
            if (v.env <= 0.0f) { v.zone = nullptr; break; }

            const float s = sampleAt(v, v.pos) * v.env * level;
            left[i] += s * v.peakL;
            if (split) right[i] += s * v.peakR;
            else left[i] += s * v.peakR;

            v.pos += v.step * v.bend;
            if (v.looping && v.pos >= (double) v.loopEnd)
                v.pos -= (double) (v.loopEnd - v.loopStart);
            if (v.pos >= (double) v.end) { v.zone = nullptr; break; }
        }
    }
}

}
