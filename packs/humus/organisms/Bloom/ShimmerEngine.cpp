// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "ShimmerEngine.h"

#include <cmath>

#include "hum/dsp/DspMath.h"

namespace sv
{

namespace
{

    constexpr float kCombMs[ShimmerEngine::kNumCombs] =
        { 25.31f, 26.94f, 28.96f, 30.75f, 32.24f, 33.81f, 35.31f, 36.67f };

    constexpr float kAllpassMs[ShimmerEngine::kNumAllpass] = { 5.10f, 7.73f, 12.61f, 10.00f };

    constexpr float kStereoOffsetMs = 0.52f;

    constexpr float kModeScale[4] = { 0.55f, 1.0f, 1.9f, 1.3f };

    constexpr float kColorHighCutScale[3] = { 2.0f, 1.0f, 0.5f };

    constexpr float kColorShiftToneHz[3] = { 12000.0f, 7000.0f, 3500.0f };

    constexpr float kMaxCombMs    = 200.0f;
    constexpr float kMaxAllpassMs = 45.0f;

    constexpr float kInputGain = 0.5f;

    constexpr float kWetMakeup = 2.0f;

#ifndef SV_FEEDBACK_CEILING
 #define SV_FEEDBACK_CEILING 0.30f
#endif
    constexpr float kFeedbackCeiling = SV_FEEDBACK_CEILING;

#ifndef SV_LOOP_MARGIN_PLAIN
 #define SV_LOOP_MARGIN_PLAIN 0.80f
#endif
#ifndef SV_LOOP_MARGIN_SHIFTED
 #define SV_LOOP_MARGIN_SHIFTED 1.00f
#endif
    constexpr float kLoopMarginPlain = SV_LOOP_MARGIN_PLAIN;
    constexpr float kLoopMarginShifted = SV_LOOP_MARGIN_SHIFTED;

#ifndef SV_COMB_FB_BASE
 #define SV_COMB_FB_BASE 0.45f
#endif
#ifndef SV_COMB_FB_RANGE
 #define SV_COMB_FB_RANGE 0.25f
#endif
    inline float combFeedbackFor (float feedback) noexcept
    {
        return SV_COMB_FB_BASE + SV_COMB_FB_RANGE * feedback;
    }

    inline float clampf (float v, float lo, float hi) noexcept
    {
        return v < lo ? lo : (v > hi ? hi : v);
    }

    inline float softClip (float x) noexcept
    {
        x = clampf (x, -3.0f, 3.0f);
        const float x2 = x * x;
        return x * (27.0f + x2) / (27.0f + 9.0f * x2);
    }

    inline int msToSamples (float ms, double sampleRate) noexcept
    {
        return static_cast<int> (ms * 0.001f * static_cast<float> (sampleRate)) + 4;
    }
}

void ShimmerEngine::prepare (double newSampleRate, int )
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : hum::kDefaultSampleRate;

    const int maxComb    = msToSamples (kMaxCombMs, sampleRate);
    const int maxAllpass = msToSamples (kMaxAllpassMs, sampleRate);

    for (int ch = 0; ch < kNumChannels; ++ch)
    {
        for (int i = 0; i < kNumCombs; ++i)
        {
            combs[ch][i].prepare (maxComb);
            combDelay[ch][i].prepare (60.0f, sampleRate);
        }

        for (int i = 0; i < kNumAllpass; ++i)
        {
            allpasses[ch][i].prepare (maxAllpass);
            allpassDelay[ch][i].prepare (60.0f, sampleRate);
        }

        shifterA[ch].prepare (sampleRate);
        shifterB[ch].prepare (sampleRate);
        lfo[ch].prepare (sampleRate);
    }

    for (Smoother* s : { &combGain, &shimmerGain, &plainGain, &tankGain, &diffusionGain,
                         &dampingCoefficient, &modDepthSamples, &mixAmount })
        s->prepare (25.0f, sampleRate);

    prepared = true;

    updateDerivedParameters();

    for (int ch = 0; ch < kNumChannels; ++ch)
    {
        for (int i = 0; i < kNumCombs; ++i)
            combDelay[ch][i].snapToTarget();
        for (int i = 0; i < kNumAllpass; ++i)
            allpassDelay[ch][i].snapToTarget();
    }

    for (Smoother* s : { &combGain, &shimmerGain, &plainGain, &tankGain, &diffusionGain,
                         &dampingCoefficient, &modDepthSamples, &mixAmount })
        s->snapToTarget();

    reset();
}

void ShimmerEngine::reset()
{
    for (int ch = 0; ch < kNumChannels; ++ch)
    {
        for (int i = 0; i < kNumCombs; ++i)
            combs[ch][i].reset();
        for (int i = 0; i < kNumAllpass; ++i)
            allpasses[ch][i].reset();

        shifterA[ch].reset();
        shifterB[ch].reset();
        lowCut[ch].reset();
        highCut[ch].reset();
        shiftTone[ch].reset();
        dcBlocker[ch].reset();
        lfo[ch].reset();

        feedbackState[ch] = 0.0f;
    }
}

void ShimmerEngine::setParams (const EngineParams& newParams)
{
    const bool shiftModeChanged = newParams.shiftMode != params.shiftMode;

    params = newParams;

    if (shiftModeChanged)
    {
        for (int ch = 0; ch < kNumChannels; ++ch)
        {
            shifterA[ch].reset();
            shifterB[ch].reset();
        }
    }

    if (prepared)
        updateDerivedParameters();
}

void ShimmerEngine::updateDerivedParameters()
{
    const int reverbModeIndex = std::min (std::max (static_cast<int> (params.reverbMode), 0), 3);
    const int colorModeIndex  = std::min (std::max (static_cast<int> (params.colorMode), 0), 2);

    const float sizeScale = 0.35f + 1.65f * clampf (params.size, 0.0f, 1.0f);
    const float scale     = kModeScale[reverbModeIndex] * sizeScale;
    const float apScale   = std::min (scale, 1.6f);

    const float samplesPerMs = 0.001f * static_cast<float> (sampleRate);

    for (int ch = 0; ch < kNumChannels; ++ch)
    {
        const float stereoOffset = (ch == 1) ? kStereoOffsetMs * samplesPerMs : 0.0f;

        for (int i = 0; i < kNumCombs; ++i)
            combDelay[ch][i].setTarget (kCombMs[i] * scale * samplesPerMs + stereoOffset);

        const float apStretch = (ch == 1) ? 1.03f : 1.0f;
        for (int i = 0; i < kNumAllpass; ++i)
            allpassDelay[ch][i].setTarget (kAllpassMs[i] * apScale * apStretch * samplesPerMs);
    }

    const float fb = clampf (params.feedback, 0.0f, 1.0f);
    const float cGain = combFeedbackFor (fb);
    combGain.setTarget (cGain);

    tankGain.setTarget (std::sqrt ((1.0f - cGain * cGain) / static_cast<float> (kNumCombs))
                        * kWetMakeup);

    const float blend = (params.shiftMode == ShiftMode::off)
                            ? 0.0f
                            : clampf (params.shimmer, 0.0f, 1.0f);

    plainGain.setTarget (fb * kLoopMarginPlain / kWetMakeup);
    shimmerGain.setTarget (fb * blend * kLoopMarginShifted / kWetMakeup);

    float g = 0.15f + 0.65f * clampf (params.diffusion, 0.0f, 1.0f);
    if (params.reverbMode == ReverbMode::cloud)
        g = std::min (g + 0.08f, 0.88f);
    diffusionGain.setTarget (g);

    const float highCutHz = clampf (params.highCutHz * kColorHighCutScale[colorModeIndex],
                                    200.0f, static_cast<float> (sampleRate) * 0.49f);
    dampingCoefficient.setTarget (static_cast<float> (std::exp (-hum::kTwoPi * highCutHz / sampleRate)));

    for (int ch = 0; ch < kNumChannels; ++ch)
    {
        lowCut[ch].setCutoff (clampf (params.lowCutHz, 20.0f, 2000.0f), sampleRate);
        highCut[ch].setCutoff (highCutHz, sampleRate);
        shiftTone[ch].setCutoff (kColorShiftToneHz[colorModeIndex], sampleRate);
        lfo[ch].setRate (params.modRateHz);
    }

    modDepthSamples.setTarget (clampf (params.modDepth, 0.0f, 1.0f) * 0.002f
                               * static_cast<float> (sampleRate));

    const float semis = clampf (params.shiftSemitones, -24.0f, 24.0f);
    ratioA = PitchShifter::semitonesToRatio (semis);
    ratioB = (params.shiftMode == ShiftMode::dual)
                 ? PitchShifter::semitonesToRatio (-semis)
                 : PitchShifter::semitonesToRatio (clampf (semis * 2.0f, -24.0f, 24.0f));

    for (int ch = 0; ch < kNumChannels; ++ch)
    {
        shifterA[ch].setRatio (ratioA);
        shifterB[ch].setRatio (ratioB);
    }

    mixAmount.setTarget (clampf (params.mix, 0.0f, 1.0f));
}

float ShimmerEngine::getMaxTankDelaySamples() const noexcept
{
    float longest = 0.0f;
    for (int i = 0; i < kNumCombs; ++i)
        longest = std::max (longest, combDelay[0][i].getCurrent());
    return longest;
}

void ShimmerEngine::process (float* left, float* right, int numSamples) noexcept
{
    if (! prepared)
        return;

    float* channelData[kNumChannels] = { left, right };
    const ShiftMode shiftMode = params.shiftMode;

    for (int n = 0; n < numSamples; ++n)
    {
        const float mix        = mixAmount.next();
        const float cGain      = combGain.next();
        const float sGain      = shimmerGain.next();
        const float apG        = diffusionGain.next();
        const float damping    = dampingCoefficient.next();
        const float modSamples = modDepthSamples.next();
        const float tGain      = tankGain.next();
        const float pGain      = plainGain.next();

        float nextFeedback[kNumChannels];

        for (int ch = 0; ch < kNumChannels; ++ch)
        {
            const float dry = channelData[ch][n];

            lfo[ch].tick();

            float x = dry * kInputGain + feedbackState[ch];
            x = lowCut[ch].process (x);
            x = highCut[ch].process (x);
            x = allpasses[ch][0].process (x, allpassDelay[ch][0].next(), apG);
            x = allpasses[ch][1].process (x, allpassDelay[ch][1].next(), apG);

            float sum = 0.0f;
            for (int i = 0; i < kNumCombs; ++i)
            {
                const float d = combDelay[ch][i].next() + lfo[ch].get (i) * modSamples;
                sum += combs[ch][i].process (x, d, cGain, damping);
            }
            sum *= tGain;

            sum = allpasses[ch][2].process (sum, allpassDelay[ch][2].next(), apG);
            sum = allpasses[ch][3].process (sum, allpassDelay[ch][3].next(), apG);

            const float wet = sum;

            float shifted = 0.0f;
            if (shiftMode != ShiftMode::off)
            {
                shifted = (shiftMode == ShiftMode::single)
                              ? shifterA[ch].process (wet)
                              : 0.5f * (shifterA[ch].process (wet) + shifterB[ch].process (wet));
                shifted = shiftTone[ch].process (shifted);
            }

            float f = wet * pGain + shifted * sGain;
            f = dcBlocker[ch].process (f);

            nextFeedback[ch] = kFeedbackCeiling * softClip (f * (1.0f / kFeedbackCeiling));

            channelData[ch][n] = dry * (1.0f - mix) + wet * mix;
        }

        const float f0 = nextFeedback[0];
        const float f1 = nextFeedback[1];
        feedbackState[0] = f0 * 0.82f + f1 * 0.18f;
        feedbackState[1] = f1 * 0.82f + f0 * 0.18f;
    }
}

}
