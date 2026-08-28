#include "Acid/Acid.h"

#include <algorithm>
#include <cmath>

namespace hum {

void Acid::noteOn(int note, int vel, const Tuning& tuning) {
    const bool legato = held_ > 0 && gate_;
    ++held_;
    note_ = note;
    targetFreq_ = tuning.hz(note);
    gate_ = true;
    if (!legato) {
        envT_ = 0;
        accent_ = vel >= kAccentVelocity;
        if (params.get("Glide", 60.0) < 1.0) curFreq_ = targetFreq_;
    }
}

void Acid::noteOff(int note) {
    if (held_ > 0) --held_;
    if (note == note_ && held_ == 0) gate_ = false;
}

void Acid::process(const float* const*, int, float* const* out, int numOut,
                   int numSamples, const Transport& transport) {
    if (numOut == 0) return;
    float* o = out[0];
    for (int c = 1; c < numOut; ++c) std::fill(out[c], out[c] + numSamples, 0.0f);

    const double sr = sampleRate_ > 0.0 ? sampleRate_ : 44100.0;
    const int wave = std::clamp((int) std::lround(params.get("Wave", 0.0)), 0, 3);
    const double cutoff = params.get("Cutoff", 900.0);
    const double res = std::max(0.5, params.get("Resonance", 6.0));
    const double envMod = params.get("EnvMod", 0.6);
    const double decay = std::max(10.0, params.get("Decay", 300.0)) * 0.001 * sr;
    const double accent = params.get("Accent", 0.5);
    const double glideMs = params.get("Glide", 60.0);
    const float drive = (float) params.get("Drive", 0.3);
    const float level = (float) params.get("Level", 0.8);
    const double attackMs = std::clamp(params.get("Attack", 3.0), 0.3, 30.0);
    const double ampDecayMs = params.get("AmpDecay", 1230.0);
    const double accentDecay = std::max(30.0, params.get("AccentDecay", 200.0)) * 0.001 * sr;
    const double filterFM = std::clamp(params.get("FilterFM", 0.0), 0.0, 1.0);
    const double track = std::clamp(params.get("Track", 0.0), 0.0, 1.0);
    const double glide = glideMs < 1.0 ? 1.0 : 1.0 - std::exp(-1.0 / (glideMs * 0.001 * sr));
    const double atk = 1.0 - std::exp(-1.0 / (attackMs * 0.001 * sr));
    const double pluck = ampDecayMs >= 2990.0
        ? 0.0 : 1.0 - std::exp(-1.0 / (ampDecayMs * 0.001 * sr));
    const double rel = 1.0 - std::exp(-1.0 / ((accent_ ? 0.05 : 0.008) * sr));
    const double resN = std::clamp((res - 0.5) / 11.5, 0.0, 1.0);
    const double accUp = 1.0 - std::exp(-1.0 / ((0.004 + 0.05 * resN) * sr));
    const double accDown = 1.0 - std::exp(-1.0 / (0.28 * sr));

    const int lfoWave = std::clamp((int) std::lround(params.get("LfoWave", 0.0)), 0, 5);
    const double lfoDepth = std::clamp(params.get("LfoDepth", 0.0), 0.0, 1.0);
    if (params.get("LfoSync", 0.0) >= 0.5)
        lfo_.setPeriodSamples(std::max(1.0, transport.samplesPerBeat()
                                                * std::max(0.0625, params.get("LfoBeats", 1.0))));
    else
        lfo_.setRate(std::clamp(params.get("LfoRate", 2.0), 0.05, 30.0), sr);
    double lfoVal = 0.0;

    MidiEvent ev[2 * MidiNode::kMaxMidiEventsPerBlock];
    int nEv = 0;
    {
        std::unique_lock<std::mutex> g(liveLock_, std::try_to_lock);
        if (g.owns_lock()) {
            for (int i = 0; i < liveCount_; ++i) { ev[nEv] = liveQ_[(size_t) i]; ev[nEv++].sampleOffset = 0; }
            liveCount_ = 0;
        }
    }
    for (int i = 0; i < stagedCount_; ++i) ev[nEv++] = staged_[(size_t) i];
    stagedCount_ = 0;
    std::stable_sort(ev, ev + nEv, [](const MidiEvent& a, const MidiEvent& b) {
        return a.sampleOffset < b.sampleOffset;
    });

    int ei = 0;
    int ctrl = 0;
    for (int i = 0; i < numSamples; ++i) {
        while (ei < nEv && ev[ei].sampleOffset <= i) {
            const auto& e = ev[ei++];
            const int st = e.data[0] & 0xF0;
            if (st == 0x90 && e.data[2] > 0) noteOn(e.data[1], e.data[2], transport.tuning());
            else if (st == 0x80 || (st == 0x90 && e.data[2] == 0)) noteOff(e.data[1]);
        }
        {
            const double p = lfo_.tick();
            if (lfoDepth > 0.0) {
                if (lfoWave == 5 && p < lfoPrevPhase_) {
                    lfoRng_ ^= lfoRng_ << 13; lfoRng_ ^= lfoRng_ >> 17; lfoRng_ ^= lfoRng_ << 5;
                    lfoHeld_ = (double) (lfoRng_ & 0xFFFFFF) / (double) 0x7FFFFF - 1.0;
                }
                lfoVal = lfoWave == 1 ? Lfo::triangle(p)
                       : lfoWave == 2 ? Lfo::square(p)
                       : lfoWave == 3 ? Lfo::sawUp(p)
                       : lfoWave == 4 ? Lfo::sawDown(p)
                       : lfoWave == 5 ? lfoHeld_
                                      : Lfo::sine(p);
            }
            lfoPrevPhase_ = p;
        }
        if ((ctrl++ & 15) == 0) {
            const double megDecay = accent_ ? accentDecay : decay;
            const double fenv = envT_ >= 0 ? std::exp(-(double) envT_ / megDecay) : 0.0;
            const double trackOct = track * std::log2(std::max(20.0, curFreq_) / 261.626);
            const double hz = std::clamp(
                cutoff * std::pow(2.0, envMod * 3.6 * fenv + accSweep_ * 2.4
                                           + trackOct + lfoDepth * 2.0 * lfoVal),
                20.0, sr * 0.45);
            lp_.setTarget(hz, resN);
        }
        if (envT_ >= 0) ++envT_;
        {
            const double accEnv = accent_ && envT_ >= 0
                ? accent * std::exp(-(double) envT_ / accentDecay) : 0.0;
            accSweep_ += (accEnv > accSweep_ ? accUp : accDown) * (accEnv - accSweep_);
        }
        curFreq_ += (targetFreq_ - curFreq_) * glide;
        if (gate_) {
            const bool attacking = envT_ >= 0 && envT_ < (long) (0.010 * sr);
            amp_ += attacking ? (1.0 - amp_) * atk : (0.0 - amp_) * pluck;
        } else {
            amp_ += (0.0 - amp_) * rel;
        }

        const double dt = curFreq_ / sr;
        float s;
        switch (wave) {
            case 1:  s = 0.60f * osc_.square(dt); break;
            case 2:  s = 0.70f * osc_.pulse(dt, 0.25); break;
            case 3:  s = 0.78f * osc_.tri(dt); break;
            default: s = osc_.saw(dt); break;
        }
        const float preGain = (float) std::pow(66.6, std::pow((double) drive, 1.7));
        const double fmOct = filterFM > 0.0 ? filterFM * 1.5 * (double) lastOut_ : 0.0;
        s = lp_.process(s * preGain, fmOct);
        const float accGain = accent_ ? (float) (1.0 + 0.7 * accent) : 1.0f;
        s = std::tanh(s * (1.0f + 1.2f * drive) * accGain);
        const float y = s * (float) amp_ * level;
        lastOut_ = y;
        o[i] = y;
    }
}

}
