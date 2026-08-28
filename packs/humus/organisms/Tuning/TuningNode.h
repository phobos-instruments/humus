#pragma once
#include <atomic>
#include <mutex>

#include "hum/Capabilities.h"
#include "hum/Organism.h"
#include "hum/Tuning.h"

namespace hum {

class TuningNode : public Organism, public MidiNode, public TuningProvider, public FileLoader {
public:
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    int numMidiInputs() const override { return 0; }
    int numMidiOutputs() const override { return 1; }

    void prepare(double sampleRate, int) override {
        sampleRate_ = sampleRate;
        parseSclFromParam();
        rebuild();
    }

    void deliverMidi(int, const MidiEvent*, int) override {}
    int collectMidi(int, MidiEvent*, int) override { return 0; }

    void process(const float* const*, int, float* const*, int, int,
                 const Transport&) override {}

    const Tuning& tuning() const override { return tuning_; }

    void loadFromFile(const std::string& uri) override;

    void refreshTuning() override {
        if (sclFresh_.load(std::memory_order_acquire)) {
            if (sclLock_.try_lock()) {
                scl_ = sclStaged_;
                sclFresh_.store(false, std::memory_order_release);
                sclLock_.unlock();
                lastPreset_ = -1.0;
            }
        }
        if (params.get("Preset", 0.0) != lastPreset_
            || params.get("Divisions", 12.0) != lastDivisions_
            || params.get("Root", 69.0) != lastRoot_
            || params.get("RootHz", 440.0) != lastRootHz_
            || params.get("Map", 0.0) != lastMap_
            || params.get("Plugins", 0.0) != lastPlugins_)
            rebuild();
    }

private:
    void rebuild();
    void parseSclFromParam();

    Tuning tuning_;
    Tuning scl_;
    Tuning sclStaged_;
    std::atomic<bool> sclFresh_{false};
    std::mutex sclLock_;
    double lastPreset_ = -1.0, lastDivisions_ = -1.0, lastRoot_ = -1.0, lastRootHz_ = -1.0,
           lastMap_ = -1.0, lastPlugins_ = -1.0;
};

}
