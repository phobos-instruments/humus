#pragma once

#include "dsp/CombFilter.h"
#include "dsp/Filters.h"
#include "dsp/Modulator.h"
#include "dsp/PitchShifter.h"

namespace sv
{

enum class ReverbMode { bloom = 0, hall, cathedral, cloud, numModes };
enum class ShiftMode  { single = 0, dual, stacked, off, numModes };
enum class ColorMode  { bright = 0, neutral, dark, numModes };

struct EngineParams
{
    float mix            = 0.5f;
    float shiftSemitones = 12.0f;
    float feedback       = 0.5f;
    float diffusion      = 0.7f;
    float size           = 0.5f;
    float lowCutHz       = 100.0f;
    float highCutHz      = 8000.0f;
    float modRateHz      = 0.5f;
    float modDepth       = 0.25f;

    float shimmer        = 0.6f;
    ReverbMode reverbMode = ReverbMode::hall;
    ShiftMode  shiftMode  = ShiftMode::single;
    ColorMode  colorMode  = ColorMode::neutral;
};

class ShimmerEngine
{
public:
    static constexpr int kNumCombs    = 8;
    static constexpr int kNumAllpass  = 4;
    static constexpr int kNumChannels = 2;

    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    void setParams (const EngineParams& newParams);
    const EngineParams& getParams() const noexcept { return params; }

    void process (float* left, float* right, int numSamples) noexcept;

    double getSampleRate() const noexcept { return sampleRate; }

    float getMaxTankDelaySamples() const noexcept;

private:
    void updateDerivedParameters();

    double sampleRate = 44100.0;
    bool prepared = false;

    EngineParams params;

    CombFilter combs[kNumChannels][kNumCombs];
    AllpassDiffuser allpasses[kNumChannels][kNumAllpass];
    PitchShifter shifterA[kNumChannels];
    PitchShifter shifterB[kNumChannels];
    OnePoleHighpass lowCut[kNumChannels];
    OnePoleLowpass highCut[kNumChannels];
    OnePoleLowpass shiftTone[kNumChannels];
    DCBlocker dcBlocker[kNumChannels];
    ModulatorBank<kNumCombs> lfo[kNumChannels];

    float feedbackState[kNumChannels] = { 0.0f, 0.0f };

    Smoother combDelay[kNumChannels][kNumCombs];
    Smoother allpassDelay[kNumChannels][kNumAllpass];
    Smoother combGain;
    Smoother shimmerGain;
    Smoother plainGain;
    Smoother tankGain;
    Smoother diffusionGain;
    Smoother dampingCoefficient;
    Smoother modDepthSamples;
    Smoother mixAmount;

    float ratioA = 2.0f;
    float ratioB = 0.5f;
};

}
