#include "DNA/DNA.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "hum/Swing.h"

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {

constexpr const char* kBaseParam[4] = {"Base A", "Base C", "Base G", "Base T"};

constexpr double kStepsPerBeat[] = {
    4.0,
    2.0,
    8.0,
    3.0,
    6.0,
    4.0 / 3.0,
    1.0,
    8.0 / 3.0,
    1.5,
    0.5,
    0.25,
    16.0,
};
constexpr int kNumRates = (int) (sizeof(kStepsPerBeat) / sizeof(kStepsPerBeat[0]));
}

void DNA::grow(int seed) {
    rng_ = (std::uint32_t) (seed * 2654435761u + 1u);
    for (auto& b : genome_) b = (int) (rnd() & 3u);
    lastSeed_ = seed;
}

void DNA::emit(int offset, bool on, int note, int vel) {
    if (outCount_ >= (int) outEvents_.size()) return;
    MidiEvent e;
    e.data[0] = on ? 0x90 : 0x80;
    e.data[1] = (unsigned char) std::clamp(note, 0, kMidiMax);
    e.data[2] = (unsigned char) (on ? vel : 0);
    e.size = 3;
    e.sampleOffset = offset;
    outEvents_[(size_t) outCount_++] = e;
}

void DNA::process(const float* const*, int, float* const*, int,
                  int numSamples, const Transport& transport) {
    const double sr = sampleRate_ > 0.0 ? sampleRate_ : kDefaultSampleRate;
    const int seed = (int) params.get("Seed", 1.0);
    if (seed != lastSeed_) grow(seed);

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
    if (!transport.playing()) return;

    const int rate = std::clamp((int) params.get("Rate", 0.0), 0, kNumRates - 1);
    const double stepsPerBeat = kStepsPerBeat[rate];
    const int root = (int) params.get("Root", 45.0);
    const Chord chord = Chord::byId((int) params.get("Chord", 0.0));
    double baseCents[4];
    for (int b = 0; b < 4; ++b) {
        const double ov = params.get(kBaseParam[b], -1.0);
        baseCents[b] = ov >= 0.0 ? std::min(ov, 12.0) * 100.0 : chord.cents(b);
    }
    const auto& tuning = transport.tuning();
    const double gate = params.get("Gate", 0.6);
    const double mutate = params.get("Mutate", 0.15);
    const double rest = params.get("Rest", 0.25);
    const int mute = (int) params.get("Mute", 0.0);
    const int octaves = std::clamp((int) params.get("Octaves", 2.0), 1, 3);
    const int vel = (int) params.get("Velocity", 100.0);

    const long bar = (long) std::floor(transport.beats() / std::max(1.0, transport.beatsPerBar()));
    if (bar != lastBar_) {
        lastBar_ = bar;
        if ((rnd() & 0xFFFF) / 65536.0 < mutate)
            genome_[(size_t) (rnd() % kBases)] = (int) (rnd() & 3u);
    }

    const double stepsPerSec = transport.tempo() / kSecondsPerMinute * stepsPerBeat;
    const double step0 = transport.beats() * stepsPerBeat;
    const long stepLen = (long) (sr / stepsPerSec);
    const double ticksPerStep = (double) Pattern::kTicksPerBeat / stepsPerBeat;
    const auto groove = swing::resolve(params, transport, (int) std::lround(ticksPerStep));
    double next = std::ceil(step0 - 1e-9) - 1.0;
    while (true) {
        const double pos = next + swing::delaySteps(next, ticksPerStep, groove);
        const double off = (pos - step0) / stepsPerSec * sr;
        if (off >= numSamples) break;
        if (off < 0.0 || next < 0.0) { next += 1.0; continue; }
        const long idx = (long) std::llround(next);
        const int slot = (int) (idx % kBases);
        if (((mute >> slot) & 1) != 0) { next += 1.0; continue; }
        const int base = genome_[(size_t) slot];
        std::uint32_t h = (std::uint32_t) (idx * 2246822519u) ^ (std::uint32_t) (seed * 374761393u);
        h ^= h >> 16;
        h *= 0x85ebca6bu;
        h ^= h >> 13;
        h *= 0xc2b2ae35u;
        h ^= h >> 16;
        if ((h & 0xFFFF) / 65536.0 >= rest) {
            const int lift = octaves > 1 && ((h >> 20) % (std::uint32_t) octaves) == 1
                                 ? tuning.notesPerPeriod() : 0;
            const int note = root + Scale::nearestNote(tuning, baseCents[base]) + lift;
            const int at = (int) std::max(0.0, off);
            emit(at, true, note, vel);
            if (offCount_ < (int) offs_.size())
                offs_[(size_t) offCount_++] = {note, (long) (at + std::max(32.0, gate * (double) stepLen))};
        }
        next += 1.0;
    }
}

}
