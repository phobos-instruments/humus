// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "DelayLine.h"

namespace sv
{

class CombFilter
{
public:
    void prepare (int maxDelaySamples)
    {
        delay.prepare (maxDelaySamples);
        lowpassState = 0.0f;
    }

    void reset() noexcept
    {
        delay.reset();
        lowpassState = 0.0f;
    }

    float process (float input, float delaySamples, float feedback, float damping) noexcept
    {
        const float y = delay.read (delaySamples);
        lowpassState = flushDenormal (y * (1.0f - damping) + lowpassState * damping);
        delay.write (input + lowpassState * feedback);
        return y;
    }

private:
    DelayLine delay;
    float lowpassState = 0.0f;
};

class AllpassDiffuser
{
public:
    void prepare (int maxDelaySamples) { delay.prepare (maxDelaySamples); }
    void reset() noexcept { delay.reset(); }

    float process (float input, float delaySamples, float g) noexcept
    {
        const float delayed = delay.read (delaySamples);
        const float v = input + g * delayed;
        delay.write (flushDenormal (v));
        return delayed - g * v;
    }

private:
    DelayLine delay;
};

}
