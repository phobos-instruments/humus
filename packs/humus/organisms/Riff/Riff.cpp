#include "Riff/Riff.h"

#include <algorithm>
#include <cmath>

#include "hum/Swing.h"

namespace hum {

void Riff::emit(int offset, bool on, int note, int vel) {
    if (outCount_ >= (int) outEvents_.size()) return;
    MidiEvent e;
    e.data[0] = on ? 0x90 : 0x80;
    e.data[1] = (unsigned char) std::clamp(note, 0, 127);
    e.data[2] = (unsigned char) (on ? vel : 0);
    e.size = 3;
    e.sampleOffset = offset;
    outEvents_[(size_t) outCount_++] = e;
}

void Riff::process(const float* const*, int, float* const*, int,
                   int numSamples, const Transport& transport) {
    const double sr = sampleRate_ > 0.0 ? sampleRate_ : 44100.0;
    if (stepsDirty_) {
        steps_.clear();
        for (const auto& ch : pattern_.channels)
            if (ch.type == "bassline-pattern-matrix") { steps_ = decodeBassline(ch.matrix); break; }
        stepsPerBeat_ = (double) Pattern::kTicksPerBeat
                      / (double) stepTicksFor(pattern_.matrixResolution);
        stepsDirty_ = false;
    }

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
    if (!transport.playing() || steps_.empty()) return;

    const int transpose = (int) params.get("Transpose", 0.0);

    const double gate = std::clamp(params.get("Gate", 0.55), 0.05, 1.0);
    const int vel = (int) params.get("Velocity", 96.0);

    const double stepsPerSec = transport.tempo() / 60.0 * stepsPerBeat_;
    const long stepLen = (long) (sr / stepsPerSec);
    const double ticksPerStep = (double) Pattern::kTicksPerBeat / stepsPerBeat_;
    const auto groove = swing::resolve(params, transport, (int) std::lround(ticksPerStep));
    const double swingFrac = swing::cellFraction(groove.amount);
    const double step0 = transport.beats() * stepsPerBeat_;
    const double stepEnd = step0 + (double) numSamples * stepsPerSec / sr;

    for (long k = (long) std::floor(step0 - swingFrac) - 1; k < (long) std::ceil(stepEnd) + 1; ++k) {
        const double pos = (double) k + swing::delaySteps((double) k, ticksPerStep, groove);
        if (pos < step0 || pos >= stepEnd || k < 0) continue;
        const auto& s = steps_[(size_t) (k % (long) steps_.size())];
        if (!s.gate) continue;
        const int at = std::min(numSamples - 1, (int) ((pos - step0) / stepsPerSec * sr));
        const int note = std::clamp(s.note + transpose, 0, 127);
        emit(at, true, note, s.accent ? 118 : vel);
        if (offCount_ < (int) offs_.size()) {
            const long len = s.slide ? stepLen + (long) (0.003 * sr)
                                     : (long) (gate * (double) stepLen);
            offs_[(size_t) offCount_++] = {note, (long) at + std::max((long) 32, len)};
        }
    }
}

}
