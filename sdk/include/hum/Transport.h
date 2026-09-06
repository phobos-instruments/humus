#pragma once
#include <cstdint>
#include <string>

#include "hum/Swing.h"
#include "hum/Tuning.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class Transport {
public:
    void prepare(double sampleRate, double tempoBpm) {
        sampleRate_ = sampleRate;
        tempoBpm_ = tempoBpm;
    }

    double sampleRate() const { return sampleRate_; }
    double tempo() const { return tempoBpm_; }
    void setTempo(double bpm) { if (bpm > 0.0) tempoBpm_ = bpm; }
    bool playing() const { return playing_; }
    void setPlaying(bool p) { playing_ = p; }

    double beatsPerBar() const { return beatsPerBar_; }
    void setBeatsPerBar(double b) { if (b > 0.0) beatsPerBar_ = b; }

    int64_t samplePosition() const { return samplePos_; }
    std::uint32_t seekStamp() const { return seekStamp_; }
    void setSamplePosition(int64_t p) {
        samplePos_ = p < 0 ? 0 : p;
        beatPos_ = (double) samplePos_ / samplesPerBeat();
        ++seekStamp_;
    }
    void setBeatPosition(double beats) {
        beatPos_ = beats < 0.0 ? 0.0 : beats;
        samplePos_ = (int64_t) (beatPos_ * samplesPerBeat());
        ++seekStamp_;
    }
    void restorePosition(int64_t samplePos, double beats) {
        samplePos_ = samplePos < 0 ? 0 : samplePos;
        beatPos_ = beats < 0.0 ? 0.0 : beats;
        ++seekStamp_;
    }
    void advance(int numSamples) {
        if (!playing_) return;
        samplePos_ += numSamples;
        beatPos_ += (double) numSamples / samplesPerBeat();
        if (loopEnabled_ && loopEndBeat_ > loopStartBeat_ && beatPos_ >= loopEndBeat_) {
            const double span = loopEndBeat_ - loopStartBeat_;
            double wrapped = beatPos_ - span;
            while (wrapped >= loopEndBeat_) wrapped -= span;
            samplePos_ -= (int64_t) ((beatPos_ - wrapped) * samplesPerBeat());
            if (samplePos_ < 0) samplePos_ = 0;
            beatPos_ = wrapped;
        }
    }
    void reset() { samplePos_ = 0; beatPos_ = 0.0; }

    double loopStartBeat() const { return loopStartBeat_; }
    double loopEndBeat() const { return loopEndBeat_; }
    bool loopEnabled() const { return loopEnabled_; }

    void setLoop(double startBeat, double endBeat, bool enabled) {
        loopStartBeat_ = startBeat < 0.0 ? 0.0 : startBeat;
        loopEndBeat_ = endBeat;
        loopEnabled_ = enabled;
    }

    double samplesPerBeat() const { return 60.0 / tempoBpm_ * sampleRate_; }

    const swing::Groove& groove() const { return groove_; }
    void setGroove(swing::Groove g) { groove_ = g; }

    const Tuning& tuning() const { return active_ ? *active_ : tuning_; }

    void setTuning(Tuning t) { tuning_ = t; }
    const Tuning& defaultTuning() const { return tuning_; }

    void setActiveTuning(const Tuning* t) { active_ = t; }

    double beats() const { return beatPos_; }

    int bar() const { return 1 + (int) (beats() / beatsPerBar_); }
    double beatInBar() const {
        double b = beats();
        return 1.0 + (b - (double) (long) (b / beatsPerBar_) * beatsPerBar_);
    }

    double rhythmicUnitToSamples(const std::string& unit) const;

    void setLiveInput(const float* const* in, int numChannels, int numSamples) {
        liveIn_ = in;
        liveInChannels_ = in ? numChannels : 0;
        liveInSamples_ = in ? numSamples : 0;
    }
    int liveInputChannels() const { return liveInChannels_; }
    int liveInputLength() const { return liveInSamples_; }
    const float* liveInput(int channel) const {
        return (liveIn_ && channel >= 0 && channel < liveInChannels_) ? liveIn_[channel] : nullptr;
    }

private:
    double sampleRate_ = kDefaultSampleRate;
    double tempoBpm_ = 120.0;
    double beatsPerBar_ = 4.0;
    int64_t samplePos_ = 0;
    double beatPos_ = 0.0;
    std::uint32_t seekStamp_ = 0;
    bool playing_ = true;
    double loopStartBeat_ = 0.0;
    double loopEndBeat_ = 0.0;
    bool loopEnabled_ = false;
    swing::Groove groove_;
    Tuning tuning_;
    const Tuning* active_ = nullptr;

    const float* const* liveIn_ = nullptr;
    int liveInChannels_ = 0;
    int liveInSamples_ = 0;
};

}
