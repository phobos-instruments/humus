#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include "hum/dsp/DspMath.h"

namespace sv
{

constexpr float kPi = hum::kPiF;
constexpr float kTwoPi = 2.0f * kPi;

inline float flushDenormal (float x) noexcept
{
    return (std::abs (x) < 1.0e-15f) ? 0.0f : x;
}

inline int nextPowerOfTwo (int n) noexcept
{
    int p = 1;
    while (p < n)
        p <<= 1;
    return p;
}

class DelayLine
{
public:
    void prepare (int maxDelaySamples)
    {
        size = nextPowerOfTwo (std::max (16, maxDelaySamples + 4));
        mask = size - 1;
        buffer.assign (static_cast<size_t> (size), 0.0f);
        writeIndex = 0;
    }

    void reset() noexcept
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        writeIndex = 0;
    }

    void write (float x) noexcept
    {
        buffer[static_cast<size_t> (writeIndex)] = x;
        writeIndex = (writeIndex + 1) & mask;
    }

    float read (float delaySamples) const noexcept
    {
        delaySamples = std::min (std::max (delaySamples, 1.0f), static_cast<float> (size - 2));

        const float readPos = static_cast<float> (writeIndex) - delaySamples;
        const int i0 = static_cast<int> (std::floor (readPos));
        const float frac = readPos - static_cast<float> (i0);

        const float a = buffer[static_cast<size_t> (i0 & mask)];
        const float b = buffer[static_cast<size_t> ((i0 + 1) & mask)];
        return a + frac * (b - a);
    }

    int getSize() const noexcept { return size; }

private:
    std::vector<float> buffer;
    int size = 0;
    int mask = 0;
    int writeIndex = 0;
};

class Smoother
{
public:
    void prepare (float timeMs, double sampleRate) noexcept
    {
        const double samples = std::max (1.0, timeMs * 0.001 * sampleRate);
        coefficient = static_cast<float> (std::exp (-1.0 / samples));
    }

    void snap (float value) noexcept { current = target = value; }
    void setTarget (float value) noexcept { target = value; }

    void snapToTarget() noexcept { current = target; }

    float next() noexcept
    {
        current = target + (current - target) * coefficient;
        return flushDenormal (current);
    }

    float getCurrent() const noexcept { return current; }

private:
    float current = 0.0f;
    float target = 0.0f;
    float coefficient = 0.0f;
};

}
