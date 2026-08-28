#pragma once
#include <memory>

namespace hum {

class LinkSync {
public:
    LinkSync();
    ~LinkSync();

    void setEnabled(bool on);
    bool enabled() const;
    int numPeers() const;

    void proposeTempo(double bpm);

    double sessionTempo() const;

    void setStartStopSyncEnabled(bool on);
    bool sessionPlaying() const;
    void proposePlaying(bool on);

    struct Pulse {
        double bpm = 0.0;
        double phase = 0.0;
    };
    Pulse capture(int numSamples, double sampleRate, int outputLatencySamples,
                  double quantum);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}
