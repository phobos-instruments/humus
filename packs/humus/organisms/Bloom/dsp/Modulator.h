#pragma once
#include <cmath>

#include "DelayLine.h"
#include "hum/dsp/DspMath.h"

namespace sv
{

template <int NumOutputs>
class ModulatorBank
{
public:
    void prepare (double sr) noexcept
    {
        sampleRate = sr;
        phase = 0.0f;
        updateIncrement();
    }

    void reset() noexcept { phase = 0.0f; }

    void setRate (float hz) noexcept
    {
        rateHz = hz;
        updateIncrement();
    }

    void tick() noexcept
    {
        phase += increment;
        if (phase >= 1.0f)
            phase -= 1.0f;
    }

    float get (int index) const noexcept
    {
        float p = phase + static_cast<float> (index) / static_cast<float> (NumOutputs);
        if (p >= 1.0f)
            p -= 1.0f;
        return std::sin (hum::kTwoPi * p);
    }

private:
    void updateIncrement() noexcept
    {
        increment = (sampleRate > 0.0) ? static_cast<float> (rateHz / sampleRate) : 0.0f;
    }

    double sampleRate = hum::kDefaultSampleRate;
    float rateHz = 0.5f;
    float increment = 0.0f;
    float phase = 0.0f;
};

}
