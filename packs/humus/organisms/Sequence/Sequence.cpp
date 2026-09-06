#include "Sequence/Sequence.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "hum/PatternMatrix.h"
#include "hum/Swing.h"

#include "hum/dsp/DspMath.h"

namespace hum {

void Sequence::emit(int port, int offset, bool on, int note, int vel) {
    MidiEvent e;
    e.data[0] = on ? 0x90 : 0x80;
    e.data[1] = (unsigned char) std::clamp(note, 0, kMidiMax);
    e.data[2] = (unsigned char) (on ? vel : 0);
    e.size = 3;
    e.sampleOffset = offset;
    for (int p : {port, kMaster}) {
        auto& count = outCount_[(size_t) p];
        if (count < (int) outEvents_[(size_t) p].size()) outEvents_[(size_t) p][(size_t) count++] = e;
    }
}

void Sequence::process(const float* const*, int, float* const*, int,
                       int numSamples, const Transport& transport) {
    const double sr = sampleRate_ > 0.0 ? sampleRate_ : kDefaultSampleRate;

    for (int i = 0; i < offCount_;) {
        if (offs_[(size_t) i].samplesLeft < numSamples) {
            emit(offs_[(size_t) i].port, (int) offs_[(size_t) i].samplesLeft, false,
                 offs_[(size_t) i].note, 0);
            offs_[(size_t) i] = offs_[(size_t) offCount_ - 1];
            --offCount_;
        } else {
            offs_[(size_t) i].samplesLeft -= numSamples;
            ++i;
        }
    }
    if (!transport.playing() || !pattern_.present || pattern_.duration <= 0) return;

    const double gate = std::clamp(params.get("Gate", 0.5), 0.05, 1.0);
    const int duration = pattern_.duration;
    const double samplesPerTick =
        sr / (transport.tempo() / kSecondsPerMinute) / (double) Pattern::kTicksPerBeat;
    const double tickStart = transport.beats() * Pattern::kTicksPerBeat;
    const double tickEnd = tickStart + (double) numSamples / samplesPerTick;
    const int stepTicks = Pattern::kTicksPerBeat / 4;
    const auto groove = swing::resolve(params, transport, stepTicks);

    static const double kGmDefault[kRows] = {36, 35, 39, 42, 46, 38, 37, 45};
    const auto rows = pattern_.triggerChannels();
    const int base = kRows * std::clamp((int) params.get("Bank", 0.0), 0, kPatternBanks - 1);
    for (int r = 0; r < kRows && base + r < (int) rows.size(); ++r) {
        const std::string sfx = std::to_string(r + 1);
        if (params.get("Enable_" + sfx, 1.0) < 0.5) continue;
        const int note =
            std::clamp((int) params.get("Note_" + sfx, kGmDefault[(size_t) r]), 0, kMidiMax);
        const int vel = std::clamp((int) std::lround(params.get("Vel_" + sfx, 0.8) * kMidiMaxD), 1, kMidiMax);
        for (int p : rows[(size_t) (base + r)]->triggers) {
            const double fireBase = (double) p + swing::delayTicks((double) p, groove);
            double k = std::ceil((tickStart - fireBase) / duration);
            for (double fire = fireBase + k * duration; fire < tickEnd; fire += duration) {
                if (fire < tickStart) continue;
                const int at = std::clamp((int) std::lround((fire - tickStart) * samplesPerTick),
                                          0, numSamples - 1);
                emit(r, at, true, note, vel);
                if (offCount_ < (int) offs_.size()) {
                    const long len =
                        std::max((long) 32, (long) (gate * stepTicks * samplesPerTick));
                    offs_[(size_t) offCount_++] = {r, note, (long) at + len};
                }
            }
        }
    }
}

}
