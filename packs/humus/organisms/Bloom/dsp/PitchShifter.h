// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <cmath>

#include "DelayLine.h"
#include "hum/dsp/DspMath.h"

namespace sv
{

class PitchShifter
{
public:
    void prepare (double sampleRate, float windowMs = 80.0f)
    {
        windowSamples = static_cast<float> (windowMs * 0.001 * sampleRate);
        delay.prepare (static_cast<int> (windowSamples * 2.0f) + 8);
        phase = 0.0f;
    }

    void reset() noexcept
    {
        delay.reset();
        phase = 0.0f;
    }

    void setRatio (float newRatio) noexcept { ratio = newRatio; }

    static float semitonesToRatio (float semitones) noexcept
    {
        return std::pow (2.0f, semitones / 12.0f);
    }

    float process (float input) noexcept
    {
        delay.write (input);

        phase -= (ratio - 1.0f);
        if (phase >= windowSamples)
            phase -= windowSamples;
        else if (phase < 0.0f)
            phase += windowSamples;

        const float pA = phase / windowSamples;
        float pB = pA + 0.5f;
        if (pB >= 1.0f)
            pB -= 1.0f;

        const float gA = 0.5f * (1.0f - std::cos (kTwoPi * pA));
        const float gB = 0.5f * (1.0f - std::cos (kTwoPi * pB));

        const float a = delay.read (phase + 1.0f);
        const float b = delay.read (pB * windowSamples + 1.0f);

        return a * gA + b * gB;
    }

private:
    DelayLine delay;
    float windowSamples = 1.0f;
    float phase = 0.0f;
    float ratio = 1.0f;
};

}
