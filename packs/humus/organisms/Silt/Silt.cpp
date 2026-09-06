#include "Silt/Silt.h"

#include <algorithm>
#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {
constexpr int kWaveBits[4] = {0x10, 0x20, 0x40, 0x80};
}

void Silt::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    reset();
}

void Silt::reset() {
    for (auto& s : sid_) {
        s.reset();
        s.set_sampling_parameters(kClock, reSID::SAMPLE_INTERPOLATE,
                                  sampleRate_ > 0.0 ? sampleRate_ : kDefaultSampleRate);
        s.set_voice_mask(0x0f);
        s.input(0);
    }
    model_.fill(-1);
    declick_.fill(1.0f);
    voiceNote_.fill(-1);
    voiceHz_.fill(0.0);
    voiceAge_.fill(0);
    age_ = 0;
}

int Silt::freqRegister(double hz) {
    const double fn = hz * 16777216.0 / kClock;
    return std::clamp((int) std::lround(fn), 0, 65535);
}

void Silt::poke(int chip, int reg, int value) {
    sid_[(size_t) chip].write((reSID::reg8) reg, (reSID::reg8) value);
}

void Silt::applyGlobals() {
    const int fc = std::clamp((int) std::lround(params.get("Cutoff", 0.7) * 2047.0), 0, 2047);
    const int res = std::clamp((int) std::lround(params.get("Resonance", 4.0)), 0, 15);
    const bool filterOn = params.get("Filter", 1.0) >= 0.5;
    const int mode = std::clamp((int) std::lround(params.get("FilterMode", 0.0)), 0, 2);
    const bool chain = chained();
    for (int c = 0; c < 2; ++c) {
        const int model = (int) params.get(c == 1 ? "ModelB" : "Model", 0.0) >= 1 ? 1 : 0;
        if (model != model_[(size_t) c]) {
            const bool firstSet = model_[(size_t) c] < 0;
            model_[(size_t) c] = model;
            sid_[(size_t) c].set_chip_model(model == 1 ? reSID::MOS8580 : reSID::MOS6581);
            if (!firstSet) declick_[(size_t) c] = 0.0f;
        }
        poke(c, 0x15, fc & 7);
        poke(c, 0x16, fc >> 3);
        const int ext = c == 1 && chain ? 0x08 : 0x00;
        poke(c, 0x17, (res << 4) | (filterOn ? 0x07 : 0x00) | ext);
        poke(c, 0x18, (0x10 << mode) | 0x0F);
    }
}

void Silt::applyVoiceShape(int chip, int v) {
    const int base = v * 7;
    const int pw = std::clamp((int) std::lround(params.get("PulseWidth", 0.5) * 4095.0),
                              0, 4095);
    poke(chip, base + 2, pw & 255);
    poke(chip, base + 3, pw >> 8);
    const int a = std::clamp((int) std::lround(params.get("Attack", 0.0)), 0, 15);
    const int d = std::clamp((int) std::lround(params.get("Decay", 8.0)), 0, 15);
    const int s = std::clamp((int) std::lround(params.get("Sustain", 10.0)), 0, 15);
    const int r = std::clamp((int) std::lround(params.get("Release", 8.0)), 0, 15);
    poke(chip, base + 5, (a << 4) | d);
    poke(chip, base + 6, (s << 4) | r);
}

void Silt::applyVoicePitch(int chip, int v) {
    const double hz = voiceHz_[(size_t) v];
    if (hz <= 0.0) return;
    const double half = params.get("TwinDetune", 6.0) / 2400.0;
    const double chz = !twin() ? hz
                     : hz * std::pow(2.0, chip == 1 ? half : -half);
    const int fn = freqRegister(chz);
    poke(chip, v * 7 + 0, fn & 255);
    poke(chip, v * 7 + 1, fn >> 8);
}

void Silt::applyVoiceControl(int chip, int v, bool gate) {
    const int wave = std::clamp((int) std::lround(params.get("Wave", 1.0)), 0, 3);
    const int ring = params.get("Ring", 0.0) >= 0.5 ? 0x04 : 0x00;
    const int sync = params.get("Sync", 0.0) >= 0.5 ? 0x02 : 0x00;
    poke(chip, v * 7 + 4, kWaveBits[wave] | ring | sync | (gate ? 0x01 : 0x00));
}

void Silt::noteOn(int note, int vel, const Tuning& tuning) {
    int v = -1;
    for (int i = 0; i < kPerChip; ++i)
        if (voiceNote_[(size_t) i] < 0) { v = i; break; }
    if (v < 0) {
        v = 0;
        for (int i = 1; i < kPerChip; ++i)
            if (voiceAge_[(size_t) i] < voiceAge_[(size_t) v]) v = i;
    }
    voiceNote_[(size_t) v] = note;
    voiceHz_[(size_t) v] = tuning.hz(note);
    voiceAge_[(size_t) v] = ++age_;

    const int chips = twin() ? 2 : 1;
    for (int c = 0; c < chips; ++c) {
        applyVoicePitch(c, v);
        applyVoiceShape(c, v);
        applyVoiceControl(c, v, true);
    }
}

void Silt::noteOff(int note) {
    for (int v = 0; v < kPerChip; ++v)
        if (voiceNote_[(size_t) v] == note) {
            for (int c = 0; c < 2; ++c) applyVoiceControl(c, v, false);
            voiceNote_[(size_t) v] = -1;
        }
}

void Silt::renderChunk(float* l, float* r, int n, float level) {
    short buf1[256], buf2[256];
    int done = 0;
    const bool two = twin();
    const bool chain = chained();
    while (done < n) {
        const int want = std::min(n - done, (int) (sizeof(buf1) / sizeof(buf1[0])));
        int got = 0;
        while (got < want) {
            reSID::cycle_count delta = (reSID::cycle_count) (kClock / 4);
            const int g = sid_[0].clock(delta, buf1 + got, want - got);
            if (g <= 0) break;
            got += g;
        }
        std::fill(buf1 + got, buf1 + want, (short) 0);
        if (two && chain) {
            for (int i = 0; i < want; ++i) {
                const int hot = std::clamp((int) buf1[i] * 4, -32768, 32767);
                sid_[1].input((short) hot);
                reSID::cycle_count delta = (reSID::cycle_count) 4096;
                if (sid_[1].clock(delta, buf2 + i, 1) <= 0) buf2[i] = 0;
            }
        } else if (two) {
            int got2 = 0;
            while (got2 < want) {
                reSID::cycle_count delta = (reSID::cycle_count) (kClock / 4);
                const int g = sid_[1].clock(delta, buf2 + got2, want - got2);
                if (g <= 0) break;
                got2 += g;
            }
            std::fill(buf2 + got2, buf2 + want, (short) 0);
        }
        const float k = level * 3.2f / 32768.0f;
        const float kMain = k / (1.0f + kTwinBleed), kBleed = kMain * kTwinBleed;
        const float rise = 1.0f / (0.04f * (float) (sampleRate_ > 0.0 ? sampleRate_
                                                                      : kDefaultSampleRate));
        for (int i = 0; i < want; ++i) {
            const float g0 = declick_[0], g1 = declick_[1];
            const float s1 = (float) buf1[i] * g0 * g0;
            const float s2 = (float) buf2[i] * g1 * g1;
            if (two) {
                l[done + i] = s1 * kMain + s2 * kBleed;
                r[done + i] = s2 * kMain + s1 * kBleed;
            } else {
                l[done + i] = s1 * k;
                r[done + i] = l[done + i];
            }
            declick_[0] = std::min(1.0f, g0 + rise);
            declick_[1] = std::min(1.0f, g1 + rise);
        }
        done += want;
    }
}

void Silt::process(const float* const*, int, float* const* out, int numOut,
                   int numSamples, const Transport& transport) {
    if (numOut == 0) return;
    float* l = out[0];
    float* r = numOut > 1 ? out[1] : out[0];

    applyGlobals();
    for (int v = 0; v < kPerChip; ++v)
        if (voiceNote_[(size_t) v] >= 0)
            for (int c = 0; c < (twin() ? 2 : 1); ++c) {
                applyVoicePitch(c, v);
                applyVoiceShape(c, v);
                applyVoiceControl(c, v, true);
            }

    MidiEvent ev[2 * MidiNode::kMaxMidiEventsPerBlock];
    int nEv = 0;
    {
        std::unique_lock<std::mutex> g(liveLock_, std::try_to_lock);
        if (g.owns_lock()) {
            for (int i = 0; i < liveCount_; ++i) {
                ev[nEv] = liveQ_[(size_t) i];
                ev[nEv++].sampleOffset = 0;
            }
            liveCount_ = 0;
        }
    }
    for (int i = 0; i < stagedCount_; ++i) ev[nEv++] = staged_[(size_t) i];
    stagedCount_ = 0;
    std::stable_sort(ev, ev + nEv, [](const MidiEvent& a, const MidiEvent& b) {
        return a.sampleOffset < b.sampleOffset;
    });

    const float level = (float) params.get("Level", 0.8);
    int at = 0;
    for (int i = 0; i <= nEv; ++i) {
        const int upTo = i < nEv ? std::clamp(ev[i].sampleOffset, at, numSamples)
                                 : numSamples;
        if (upTo > at) {
            renderChunk(l + at, r + at, upTo - at, level);
            at = upTo;
        }
        if (i < nEv) {
            const auto& e = ev[i];
            const int st = e.data[0] & 0xF0;
            if (st == 0x90 && e.data[2] > 0) noteOn(e.data[1], e.data[2], transport.tuning());
            else if (st == 0x80 || (st == 0x90 && e.data[2] == 0)) noteOff(e.data[1]);
        }
    }
}

}
