#pragma once
#include <cmath>

#include "DelayLine.h"
#include "hum/dsp/DspMath.h"

namespace sv
{

class OnePoleLowpass
{
public:
    void setCutoff (float frequencyHz, double sampleRate) noexcept
    {
        const float fc = std::min (std::max (frequencyHz, 1.0f),
                                   static_cast<float> (sampleRate) * 0.49f);
        a = std::exp (-kTwoPi * fc / static_cast<float> (sampleRate));
    }

    void reset() noexcept { z = 0.0f; }

    float process (float x) noexcept
    {
        z = flushDenormal (x * (1.0f - a) + z * a);
        return z;
    }

private:
    float a = 0.0f;
    float z = 0.0f;
};

class OnePoleHighpass
{
public:
    void setCutoff (float frequencyHz, double sampleRate) noexcept { lowpass.setCutoff (frequencyHz, sampleRate); }
    void reset() noexcept { lowpass.reset(); }
    float process (float x) noexcept { return x - lowpass.process (x); }

private:
    OnePoleLowpass lowpass;
};

class DCBlocker
{
public:
    void reset() noexcept { x1 = y1 = 0.0f; }

    float process (float x) noexcept
    {
        const float y = x - x1 + 0.9985f * y1;
        x1 = x;
        y1 = flushDenormal (y);
        return y1;
    }

private:
    float x1 = 0.0f;
    float y1 = 0.0f;
};

}
