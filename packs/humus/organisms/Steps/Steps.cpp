#include "Steps/Steps.h"

#include <algorithm>
#include <cmath>

#include "hum/Swing.h"

namespace hum {

void Steps::emit(int offset, bool on, int note, int vel) {
    if (outCount_ >= (int) outEvents_.size()) return;
    MidiEvent e;
    e.data[0] = on ? 0x90 : 0x80;
    e.data[1] = (unsigned char) std::clamp(note, 0, 127);
    e.data[2] = (unsigned char) (on ? vel : 0);
    e.size = 3;
    e.sampleOffset = offset;
    outEvents_[(size_t) outCount_++] = e;
}

void Steps::process(const float* const*, int, float* const*, int,
                    int numSamples, const Transport& transport) {
    const double sr = sampleRate_ > 0.0 ? sampleRate_ : 44100.0;
    if (stepsDirty_) {
        steps_.clear();
        ups_.clear();
        for (const auto& ch : pattern_.channels) {
            if (ch.type == "trigger-tie-matrix" && steps_.empty()) steps_ = decodeArp(ch.matrix);
            if (ch.type == "octave-row" && ups_.empty()) ups_ = decodeOctaveRow(ch.matrix);
        }
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

    const int note = (int) params.get("Note", 36.0);

    const double gate = std::clamp(params.get("Gate", 0.6), 0.05, 1.0);
    const int vel = (int) params.get("Velocity", 100.0);

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
        const size_t idx = (size_t) (k % (long) steps_.size());
        if (!steps_[idx].trigger) continue;
        const int at = std::min(numSamples - 1, (int) ((pos - step0) / stepsPerSec * sr));
        long len = (long) (gate * (double) stepLen);
        for (size_t t = 1; t < steps_.size(); ++t) {
            if (!steps_[(idx + t) % steps_.size()].tie) break;
            len = (long) t * stepLen + (long) (gate * (double) stepLen);
        }
        const int n2 = note + (idx < ups_.size() && ups_[idx] ? 12 : 0);
        emit(at, true, n2, vel);
        if (offCount_ < (int) offs_.size())
            offs_[(size_t) offCount_++] = {n2, (long) at + std::max((long) 32, len)};
    }
}

}
