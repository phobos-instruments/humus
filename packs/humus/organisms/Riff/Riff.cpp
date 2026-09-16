// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Riff/Riff.h"

#include <algorithm>
#include <cmath>

#include "hum/Swing.h"

#include "hum/dsp/DspMath.h"

namespace hum {

void Riff::emit(int offset, bool on, int note, int vel) {
    if (outCount_ >= (int) outEvents_.size()) return;
    MidiEvent e;
    e.data[0] = on ? 0x90 : 0x80;
    e.data[1] = (unsigned char) std::clamp(note, 0, kMidiMax);
    e.data[2] = (unsigned char) (on ? vel : 0);
    e.size = 3;
    e.sampleOffset = offset;
    outEvents_[(size_t) outCount_++] = e;
}

void Riff::process(const float* const*, int, float* const*, int,
                   int numSamples, const Transport& transport) {
    const int bank = std::clamp((int) params.get("Bank", 0.0), 0, kPatternBanks - 1);
    if (bank != bank_) {
        bank_ = bank;
        stepsDirty_ = true;
    }
    if (stepsDirty_) {
        steps_.clear();
        if (const auto* ch = matrixChannel(pattern_, "bassline-pattern-matrix", bank_))
            steps_ = decodeBassline(ch->matrix);
        stepsPerBeat_ = (double) Pattern::kTicksPerBeat
                      / (double) stepTicksFor(pattern_.matrixResolution);
        stepsDirty_ = false;
    }

    if (transport.playing() && !steps_.empty()) placeSteps(numSamples, transport);

    for (int i = 0; i < offCount_;) {
        if (offs_[(size_t) i].samplesLeft < numSamples) {
            emit((int) offs_[(size_t) i].samplesLeft, false, offs_[(size_t) i].note, 0);
            offs_[(size_t) i] = offs_[(size_t) offCount_ - 1];
            --offCount_;
        } else {
            offs_[(size_t) i].samplesLeft -= numSamples;
            ++i;
        }
    }
}

void Riff::placeSteps(int numSamples, const Transport& transport) {
    const double sr = sampleRate_ > 0.0 ? sampleRate_ : kDefaultSampleRate;

    const int transpose = (int) params.get("Transpose", 0.0);
    const long nudge = std::lround(params.get("Nudge", 0.0));

    const double gate = std::clamp(params.get("Gate", 0.55), 0.05, 1.0);
    const int vel = (int) params.get("Velocity", 96.0);

    const double stepsPerSec = transport.tempo() / kSecondsPerMinute * stepsPerBeat_;
    const long stepLen = (long) (sr / stepsPerSec);
    const double ticksPerStep = (double) Pattern::kTicksPerBeat / stepsPerBeat_;
    const auto groove = swing::resolve(params, transport, (int) std::lround(ticksPerStep));
    const double swingFrac = swing::cellFraction(groove.amount);
    const double step0 = transport.beats() * stepsPerBeat_;
    const double stepEnd = step0 + (double) numSamples * stepsPerSec / sr;

    const long count = (long) steps_.size();
    const auto stepAt = [&](long step) -> const BasslineStep& {
        return steps_[(size_t) (((step - nudge) % count + count) % count)];
    };

    for (long k = (long) std::floor(step0 - swingFrac) - 1; k < (long) std::ceil(stepEnd) + 1; ++k) {
        const double pos = (double) k + swing::delaySteps((double) k, ticksPerStep, groove);
        if (pos < step0 || pos >= stepEnd || k < 0) continue;
        const auto& s = stepAt(k);
        if (!s.gate) continue;
        const int at = std::min(numSamples - 1, (int) ((pos - step0) / stepsPerSec * sr));
        const int note = std::clamp(s.note + transpose, 0, kMidiMax);
        const double nextPos = (double) (k + 1) + swing::delaySteps((double) (k + 1), ticksPerStep, groove);
        const long toNext = (long) ((nextPos - pos) / stepsPerSec * sr);
        const long len = s.slide ? toNext + (long) (kSlideOverlapSeconds * sr)
                                 : (long) (gate * (double) stepLen);
        const long until = (long) at + std::max(kMinNoteSamples, len);

        const auto& before = stepAt(k - 1);
        PendingOff* held = nullptr;
        if (before.gate && before.slide && before.note == s.note)
            for (int i = 0; i < offCount_; ++i)
                if (offs_[(size_t) i].note == note) held = &offs_[(size_t) i];
        if (held != nullptr) {
            held->samplesLeft = until;
            continue;
        }

        emit(at, true, note, s.accent ? kAccentVelocity : vel);
        if (offCount_ < (int) offs_.size()) offs_[(size_t) offCount_++] = {note, until};
    }
}

}
