#pragma once
#include <array>
#include <cmath>
#include <cstdint>

#include "hum/Capabilities.h"
#include "hum/HeldNotes.h"
#include "hum/Organism.h"

namespace hum {

struct Wander {
    double pos = 1.0;
    double v0 = 0.0, v1 = 0.0;
    double value = 0.0;

    double step(double dphase, std::uint32_t& rng) {
        pos += dphase;
        while (pos >= 1.0) {
            pos -= 1.0;
            v0 = v1;
            rng ^= rng << 13;
            rng ^= rng >> 17;
            rng ^= rng << 5;
            v1 = (double) rng / 2147483648.0 - 1.0;
        }
        const double s = 0.5 - 0.5 * std::cos(3.14159265358979 * pos);
        value = v0 + (v1 - v0) * s;
        return value;
    }
};

struct TromboneGlottis {
    double timeInWaveform = 0.0, totalTime = 0.0;
    double oldFrequency = 140.0, newFrequency = 140.0;
    double targetFrequency = 140.0, smoothFrequency = 140.0;
    double oldTenseness = 0.6, newTenseness = 0.6, targetTenseness = 0.6;
    double intensity = 0.0, loudness = 1.0;
    double vibratoAmount = 0.005;
    double driftAmount = 0.4;
    double aspMod = 0.2;
    double frequency = 140.0, waveformLength = 1.0 / 140.0;
    double alpha = 0.0, e0 = 0.0, epsilon = 0.0, shift = 0.0, delta = 1.0;
    double te = 0.0, omega = 0.0;
    Wander vib1, vib2, wob1, wob2, ten1, ten2, asp1;
    std::uint32_t rng = 0x9e3779b9u;

    void reset();
    void setupWaveform(double lambda);
    double noiseModulator() const;
    double runStep(double lambda, double aspirateNoise, double sampleRate);
    void finishBlock(bool sounding, bool droneBreath, double blockTime, bool wobble);
};

struct TromboneTract {
    static constexpr int kN = 44;
    static constexpr int kNoseLength = 28;
    static constexpr int kBladeStart = 10;
    static constexpr int kTipStart = 32;
    static constexpr int kLipStart = 39;
    static constexpr int kNoseStart = kN - kNoseLength + 1;
    static constexpr int kMaxTransients = 8;

    struct Transient {
        int position = 0;
        double timeAlive = 0.0;
    };

    std::array<double, kN> R{}, L{}, diameter{}, restDiameter{}, targetDiameter{}, A{};
    std::array<double, kN + 1> junctionR{}, junctionL{}, reflection{}, newReflection{};
    std::array<double, kNoseLength> noseR{}, noseL{}, noseDiameter{}, noseA{};
    std::array<double, kNoseLength + 1> noseJunctionR{}, noseJunctionL{}, noseReflection{};
    double reflectionLeft = 0.0, reflectionRight = 0.0, reflectionNose = 0.0;
    double newReflectionLeft = 0.0, newReflectionRight = 0.0, newReflectionNose = 0.0;
    double velumTarget = 0.01;
    int lastObstruction = -1;
    double lipOutput = 0.0, noseOutput = 0.0;
    std::array<Transient, kMaxTransients> transients{};
    int transientCount = 0;

    void init();
    void setRestDiameter(double tongueIndex, double tongueDiameter);
    void applyConstriction(double index, double diam);
    void reshape(double deltaTime);
    void calculateReflections();
    void calculateNoseReflections();
    void runStep(double glottalOutput, double turbulence, double lambda,
                 double noiseMod, double constrIndex, double constrDiameter,
                 double sampleRate);
};

class PinkTrombone : public Organism, public MidiNode {
public:
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 1; }
    int numMidiInputs() const override { return 1; }
    int numMidiOutputs() const override { return 0; }

    void prepare(double sampleRate, int) override;
    void reset() override;
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    void deliverMidi(int, const MidiEvent* events, int count) override {
        stagedCount_ = std::min(count, (int) staged_.size());
        for (int i = 0; i < stagedCount_; ++i) staged_[(size_t) i] = events[i];
    }
    int collectMidi(int, MidiEvent*, int) override { return 0; }

private:
    struct Bandpass {
        double b0 = 0, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
        void set(double hz, double q, double sr);
        double process(double x);
        void clear() { x1 = x2 = y1 = y2 = 0.0; }
    };

    TromboneGlottis glottis_;
    TromboneTract tract_;
    Bandpass aspFilter_, fricFilter_;
    HeldNotes held_;
    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> staged_;
    int stagedCount_ = 0;
    double fricIntensity_ = 0.0;
    double velocity_ = 0.8;
    std::uint32_t noiseRng_ = 0x2545f491u;
};

}
