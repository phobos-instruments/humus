// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "TestGen/TestGen.h"

#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

void TestGen::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    for (auto& v : pink_) v = 0.0f;
    brown_ = 0.0f;
}

float TestGen::nextNoise() {
    rngState_ ^= rngState_ << 13;
    rngState_ ^= rngState_ >> 17;
    rngState_ ^= rngState_ << 5;
    return (float) ((double) rngState_ / 2147483648.0 - 1.0);
}

float TestGen::nextPink() {
    const float w = nextNoise();
    pink_[0] = 0.99765f * pink_[0] + w * 0.0990460f;
    pink_[1] = 0.96300f * pink_[1] + w * 0.2965164f;
    pink_[2] = 0.57000f * pink_[2] + w * 1.0526913f;
    return (pink_[0] + pink_[1] + pink_[2] + w * 0.1848f) * 0.2f;
}

float TestGen::nextBrown() {
    brown_ = 0.997f * brown_ + nextNoise() * 0.035f;
    return brown_ * 3.5f;
}

void TestGen::process(const float* const*, int,
                      float* const* out, int numOut,
                      int numSamples, const Transport&) {
    const double amp = params.get("Amplitude", 1.0);
    const double freq = params.get("Frequency", kA4Hz);
    const int shape = (int) std::lround(params.get("Waveform", 0.0));
    const double inc = 2.0 * kPi * freq / sampleRate_;

    for (int n = 0; n < numSamples; ++n) {
        float s = 0.0f;
        switch (shape) {
            case 1:  s = nextNoise() * (float) amp; break;
            case 2:  s = nextPink() * (float) amp;  break;
            case 3:  s = nextBrown() * (float) amp; break;
            default:
                s = (float) (std::sin(phase_) * amp);
                phase_ += inc;
                if (phase_ >= 2.0 * kPi) phase_ -= 2.0 * kPi;
                break;
        }
        for (int c = 0; c < numOut; ++c) out[c][n] = s;
    }
}

}
