#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>

#include "hum/Capabilities.h"
#include "hum/Organism.h"
#include "hum/SliceEdits.h"
#include "hum/dsp/SliceDetect.h"

namespace hum {

class Leafcutter : public Organism, public FileLoader, public MidiNode,
                   public LiveMidiIn, public SliceSource {
public:
    static constexpr int kVoices = 8;
    static constexpr int kBaseNote = 48;

    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 2; }
    void prepare(double sampleRate, int maxBlock) override;
    void reset() override;
    void process(const float* const* in, int numIn, float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    void loadFromFile(const std::string& uri) override;

    int numMidiInputs() const override { return 1; }
    int numMidiOutputs() const override { return 0; }
    void deliverMidi(int port, const MidiEvent* events, int count) override;
    int collectMidi(int, MidiEvent*, int) override { return 0; }
    void pushLiveMidi(const MidiEvent& e) override;

    unsigned sliceGeneration() const override { return gen_.load(std::memory_order_relaxed); }
    const std::vector<float>& slicePeaks() const override { return peaks_; }
    std::vector<float> sliceStarts() const override;
    int playingSlice() const override { return playing_.load(std::memory_order_relaxed); }

private:
    struct Voice {
        double pos = 0.0;
        double start = 0.0;
        double end = 0.0;
        double gateLeft = 0.0;
        double step = 1.0;
        float gain = 0.0f, env = 0.0f;
        float decayGain = 1.0f;
        int note = -1;
        bool releasing = false;
        bool active = false;
    };

    void applyPending();
    void triggerVoice(Voice& v, int slice, double windowOutSamples, float gain);
    void handleEvent(const MidiEvent& e);
    void renderAdd(float* left, float* right, int from, int count);
    int activeCount() const { return (int) active_.size(); }

    juce::AudioBuffer<float> buf_;
    double srcRate_ = 44100.0;
    std::vector<SliceOnset> onsets_;
    std::string uri_;

    juce::AudioBuffer<float> pendingBuf_;
    double pendingRate_ = 44100.0;
    std::vector<SliceOnset> pendingOnsets_;
    double fileBeats_ = 0.0;
    double pendingFileBeats_ = 0.0;
    juce::SpinLock swap_;
    std::atomic<bool> hasPending_{false};

    std::vector<SliceOnset> active_;
    double lastSense_ = -1.0;
    std::array<int, kMaxSliceOnsets> perm_{};
    int lastShuffle_ = 0, lastPermCount_ = -1;

    std::map<int, SliceEdit> edits_;
    std::string editsCache_;
    std::array<Voice, kVoices> voices_;
    Voice loopVoice_;
    int lastWindow_ = -1;
    double beat_ = 0.0;
    double lastLoopBeat_ = -1.0;

    std::vector<float> peaks_;
    std::vector<SliceOnset> guiOnsets_;
    int guiLen_ = 0;
    std::atomic<unsigned> gen_{0};
    std::atomic<int> playing_{-1};
    int lastMidiSlice_ = -1;

    std::array<MidiEvent, MidiNode::kMaxMidiEventsPerBlock> staged_;
    int stagedCount_ = 0;
    std::mutex liveLock_;
    std::array<MidiEvent, 64> liveQ_;
    int liveCount_ = 0;
};

}
