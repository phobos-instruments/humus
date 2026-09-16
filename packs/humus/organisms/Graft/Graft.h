// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_events/juce_events.h>

#include "Graft/GraftStitch.h"

#include "hum/caps/Files.h"
#include "hum/caps/Graph.h"
#include "hum/caps/Midi.h"
#include "hum/caps/Params.h"
#include "hum/caps/Samples.h"
#include "hum/Organism.h"
#include "hum/PitchBend.h"
#include "hum/SliceEdits.h"
#include "hum/dsp/SliceDetect.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class Graft : public Organism, public MidiNode, public FileLoader, public LiveMidiIn,
              public SliceSource, public SliceWorkbench, public TextSource,
              public ReloadOnParam, private juce::Timer {
public:
    static constexpr int kMaxVoices = 16;
    static constexpr int kPeakBins = 512;
    static constexpr int kCoalesceMs = 45;
    static constexpr int kRootNote = 60;

    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 2; }
    void prepare(double sampleRate, int maxBlock) override;
    void reset() override;
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    void loadFromFile(const std::string& uri) override;
    void restitchNow();
    bool rebuildPending() const { return isTimerRunning(); }
    void loadFrom(const OrganismState& state) override {
        Organism::loadFrom(state);
        restitchNow();
    }
    void onTextChanged(const std::string& param, const std::string&) override {
        if (param == "SliceEdits" || param == "Pins") restitchNow();
    }

    bool reloadsOn(const std::string& param) const override;

    int numMidiInputs() const override { return 1; }
    int numMidiOutputs() const override { return 0; }
    void deliverMidi(int port, const MidiEvent* events, int count) override;
    int collectMidi(int, MidiEvent*, int) override { return 0; }
    void pushLiveMidi(const MidiEvent& e) override;

    unsigned sliceGeneration() const override { return gen_.load(std::memory_order_relaxed); }
    const std::vector<float>& slicePeaks() const override { return peaks_; }
    std::vector<float> sliceStarts() const override { return guiBounds_; }
    int playingSlice() const override { return playing_.load(std::memory_order_relaxed); }

    std::string pinParam() const override { return "Pins"; }
    std::string pinValue(int index) const override;
    std::string pinAlternative(int index) const override;
    std::string pinNudged(const std::string& from, int sourceStep,
                          int fragmentStep) const override;
    std::string startParam() const override { return "Start"; }
    int sliceOrigin(int index) const override;
    int originCount() const override { return kGraftSlots; }
    std::string originName(int origin) const override;
    void auditionSlice(int index) override {
        auditionWanted_.store(index, std::memory_order_relaxed);
    }

    int textLines(std::string* out, int capacity) const override;

    double stitchedSeconds() const { return guiSeconds_; }
    double barStretch() const { return stretch_; }

private:
    struct Voice {
        double pos = 0.0;
        double leaving = 0.0;
        double step = 1.0;
        double stop = 0.0;
        float env = 0.0f;
        float gain = 0.0f;
        float releaseOverride = 0.0f;
        int note = -1;
        int stage = 0;
        bool active = false;
        bool drone = false;
        bool blending = false;
    };

    void timerCallback() override;
    void restitch(double destRate);
    void syncTextParams();
    void takeAudition();
    void publish(GraftResult&& result);
    void adoptPending();
    void handleEvent(const MidiEvent& e);
    void startVoice(Voice& v, int note, float velocity, bool drone);
    void renderAdd(float* left, float* right, int from, int count);
    void advanceEnvelope(Voice& v, float attack, float decay, float sustain, float release);
    GraftSpec specFromParams(double destRate, double tempo) const;
    bool refreshSources();

    std::vector<GraftSource> sources_{(size_t) kGraftSlots};
    std::array<std::vector<SliceOnset>, kGraftSlots> detected_;
    std::array<std::string, kGraftSlots> uris_;
    std::vector<GraftSlot> plan_;
    std::map<int, GraftSlot> pins_;
    std::map<int, SliceEdit> edits_;
    std::string appliedEdits_, appliedPins_;

    juce::AudioBuffer<float> buf_;
    juce::AudioBuffer<float> leavingBuf_;
    juce::AudioBuffer<float> pendingBuf_;
    int blendLeft_ = 0;
    int blendSpan_ = 1;
    juce::SpinLock swap_;
    std::atomic<bool> hasPending_{false};
    std::vector<float> pendingBounds_;
    std::vector<float> bounds_;

    double sampleRate_ = kDefaultSampleRate;
    std::atomic<double> lastTempo_{0.0};
    std::atomic<double> lastBeatsPerBar_{4.0};
    double stretch_ = 1.0;
    double guiSeconds_ = 0.0;

    std::array<Voice, kMaxVoices> voices_;
    PitchBend bend_;
    double bendRatio_ = 1.0;
    Voice drone_;
    Voice audition_;
    std::atomic<int> auditionWanted_{-1};

    std::vector<float> peaks_;
    std::vector<float> guiBounds_;
    std::atomic<unsigned> gen_{0};
    std::atomic<int> playing_{-1};
    std::string status_;

    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> staged_;
    int stagedCount_ = 0;
    std::mutex liveLock_;
    std::array<MidiEvent, 64> liveQ_;
    int liveCount_ = 0;
};

}
