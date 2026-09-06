#include "Harmonizer/Harmonizer.h"

#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {
enum Mode { kModeMidi = 0, kModeScale = 1 };
constexpr double kPan[Harmonizer::kVoices] = {-1.0, 1.0, -0.5, 0.5};
constexpr double kHumRate[Harmonizer::kVoices] = {0.31, 0.47, 0.23, 0.59};
const char* const kVoiceName[Harmonizer::kVoices] = {"Voice1", "Voice2", "Voice3", "Voice4"};
const char* const kLevelName[Harmonizer::kVoices] = {"Level1", "Level2", "Level3", "Level4"};
}

void Harmonizer::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    tracker_.prepare(sampleRate);
    for (auto& s : shift_) s.prepare(sampleRate, 50.0);
    reset();
}

void Harmonizer::reset() {
    for (auto& s : shift_) s.reset();
    ratio_.fill(1.0);
    targetRatio_.fill(1.0);
    gain_.fill(0.0f);
    targetGain_.fill(0.0f);
    voiceLevel_.fill(1.0f);
    humPhase_ = {0.0, 1.7, 3.1, 4.9};
    held_.clear();
    stagedCount_ = 0;
    refNote_ = -1;
    voicedHold_ = 0;
    window_ = 0;
    hz_.store(0.0f, std::memory_order_relaxed);
    clarity_.store(0.0f, std::memory_order_relaxed);
    level_.store(0.0f, std::memory_order_relaxed);
    note_.store(-1, std::memory_order_relaxed);
}

int Harmonizer::scaleStep(int baseNote, int key, int scaleId, int degrees) {
    const Scale sc = Scale::byId(scaleId);
    int semis[Scale::kMaxTones];
    int n = sc.size();
    if (n < 1) {
        n = 12;
        for (int i = 0; i < n; ++i) semis[i] = i;
    } else {
        for (int i = 0; i < n; ++i) semis[i] = (int) std::lround(sc.cents(i) / 100.0);
    }
    const int rel = baseNote - key;
    const int oct = (int) std::floor((double) rel / 12.0);
    const int pc = rel - oct * 12;
    int bestK = oct * n;
    int bestDist = 128;
    for (int i = 0; i < n; ++i)
        for (int o = -1; o <= 1; ++o) {
            const int d = std::abs(semis[i] + o * 12 - pc);
            if (d < bestDist) {
                bestDist = d;
                bestK = (oct + o) * n + i;
            }
        }
    const int k = bestK + degrees;
    const int ko = (int) std::floor((double) k / n);
    const int ki = k - ko * n;
    return key + ko * 12 + semis[ki];
}

int Harmonizer::stableNote(double exactMidi, int current) {
    if (current < 0 || std::abs(exactMidi - (double) current) > 0.65)
        return (int) std::lround(exactMidi);
    return current;
}

void Harmonizer::chooseTargets(bool voiced, int rounded) {
    const int mode = (int) std::clamp(params.get("Mode", 0.0), 0.0, 1.0);
    for (int v = 0; v < kVoices; ++v) {
        targetGain_[(size_t) v] = 0.0f;
        voiceLevel_[(size_t) v] = 1.0f;
    }
    if (!voiced) return;
    if (mode == kModeMidi) {
        int notes[kVoices];
        int n = 0;
        const int start = std::max(0, held_.count - kVoices);
        for (int i = start; i < held_.count; ++i)
            notes[n++] = held_.notes[(size_t) i];
        std::sort(notes, notes + n);
        for (int v = 0; v < n; ++v) {
            targetRatio_[(size_t) v] = std::pow(2.0, (notes[v] - rounded) / 12.0);
            targetGain_[(size_t) v] = 1.0f;
        }
        return;
    }
    const int key = (int) std::clamp(params.get("Key", 0.0), 0.0, 11.0);
    const int scaleId = (int) std::clamp(params.get("Scale", 1.0), 0.0, 14.0);
    for (int v = 0; v < kVoices; ++v) {
        const float lvl =
            (float) std::clamp(params.get(kLevelName[v], 0.0), 0.0, 1.0);
        if (lvl <= 0.0f) continue;
        const int deg = (int) std::clamp(params.get(kVoiceName[v], 0.0), -7.0, 7.0);
        const int target = scaleStep(rounded, key, scaleId, deg);
        targetRatio_[(size_t) v] = std::pow(2.0, (target - rounded) / 12.0);
        targetGain_[(size_t) v] = 1.0f;
        voiceLevel_[(size_t) v] = lvl;
    }
}

void Harmonizer::process(const float* const* in, int numIn, float* const* out, int numOut,
                    int numSamples, const Transport&) {
    if (numOut < 1) return;
    const float* src = (numIn > 0 && in && in[0]) ? in[0] : nullptr;
    for (int i = 0; i < stagedCount_; ++i) held_.apply(staged_[(size_t) i]);
    stagedCount_ = 0;

    if (src) tracker_.push(src, numSamples);
    const double hz = tracker_.pitchHz();
    const double clarity = tracker_.clarity();
    const double lvl = tracker_.level();
    const bool rawVoiced = hz > 0.0 && clarity > 0.5 && lvl > 1.0e-3;
    if (rawVoiced) {
        refNote_ = stableNote(hzToMidi(hz), refNote_);
        voicedHold_ = (int) (0.18 * sampleRate_);
        const int period = (int) (sampleRate_ / hz);
        if (std::abs(2 * period - window_) > window_ / 8) {
            window_ = 2 * period;
            for (auto& s : shift_) s.setWindow(window_);
        }
    } else {
        voicedHold_ = std::max(0, voicedHold_ - numSamples);
    }
    const bool voiced = (rawVoiced || voicedHold_ > 0) && refNote_ >= 0;
    hz_.store(rawVoiced ? (float) hz : 0.0f, std::memory_order_relaxed);
    clarity_.store((float) clarity, std::memory_order_relaxed);
    level_.store((float) lvl, std::memory_order_relaxed);
    note_.store(voiced ? refNote_ : -1, std::memory_order_relaxed);

    chooseTargets(voiced, refNote_);

    const double glideS = std::max(0.0, params.get("Glide", 40.0)) * 0.001;
    const double glideCoef =
        glideS <= 0.0 ? 1.0 : 1.0 - std::exp(-1.0 / (glideS * sampleRate_));
    const float gainCoef = 1.0f - (float) std::exp(-1.0 / (0.025 * sampleRate_));
    const double humanize = std::clamp(params.get("Humanize", 0.15), 0.0, 1.0);
    const double spread = std::clamp(params.get("Spread", 0.5), 0.0, 1.0);
    const double mix = std::clamp(params.get("Mix", 0.5), 0.0, 1.0);
    const float dry = (float) std::cos(mix * kTwoPi / 4.0);
    const float wet = (float) std::sin(mix * kTwoPi / 4.0);

    double humRatio[kVoices];
    float gL[kVoices], gR[kVoices];
    for (int v = 0; v < kVoices; ++v) {
        humPhase_[(size_t) v] += kTwoPi * kHumRate[v] * numSamples / sampleRate_;
        if (humPhase_[(size_t) v] > kTwoPi) humPhase_[(size_t) v] -= kTwoPi;
        const double cents = std::sin(humPhase_[(size_t) v]) * 14.0 * humanize;
        humRatio[v] = std::pow(2.0, cents / 1200.0);
        const double pan = kPan[v] * spread;
        gL[v] = (float) std::sqrt(0.5 * (1.0 - pan));
        gR[v] = (float) std::sqrt(0.5 * (1.0 + pan));
    }

    float* dstL = out[0];
    float* dstR = numOut > 1 ? out[1] : nullptr;
    for (int i = 0; i < numSamples; ++i) {
        const float x = src ? src[i] : 0.0f;
        float wetL = 0.0f, wetR = 0.0f;
        for (int v = 0; v < kVoices; ++v) {
            ratio_[(size_t) v] +=
                (targetRatio_[(size_t) v] * humRatio[v] - ratio_[(size_t) v]) * glideCoef;
            gain_[(size_t) v] += (targetGain_[(size_t) v] - gain_[(size_t) v]) * gainCoef;
            shift_[(size_t) v].setRatio(ratio_[(size_t) v]);
            const float y = shift_[(size_t) v].process(x)
                          * gain_[(size_t) v] * voiceLevel_[(size_t) v];
            wetL += y * gL[v];
            wetR += y * gR[v];
        }
        if (dstR) {
            dstL[i] = x * dry + wetL * wet;
            dstR[i] = x * dry + wetR * wet;
        } else {
            dstL[i] = x * dry + (wetL + wetR) * wet * kSqrtHalfF;
        }
    }
}

}
