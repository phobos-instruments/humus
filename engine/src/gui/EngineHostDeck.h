#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace hum {

class EngineHost;

class DeckHost {
public:
    explicit DeckHost(EngineHost& host) : host_(host) {}

    std::int64_t position(const std::string& name);
    std::int64_t length(const std::string& name);
    double       sampleRate(const std::string& name);
    double       effectiveBpm(const std::string& name);
    void         seek(const std::string& name, std::int64_t sample);
    void         setBend(const std::string& name, double percent);
    void         setScrub(const std::string& name, bool active, double targetSample);
    std::vector<float> waveform(const std::string& name);

    void setCue(const std::string& name);
    void jumpCue(const std::string& name);
    void setHotCue(const std::string& name, int index);
    void jumpHotCue(const std::string& name, int index);
    void clearHotCue(const std::string& name, int index);
    void setBeatLoop(const std::string& name, double beats);
    void scaleBeatLoop(const std::string& name, double factor);
    void toggleLoop(const std::string& name);
    void beginLoopRoll(const std::string& name, double beats);
    void endLoopRoll(const std::string& name);

private:
    std::int64_t quantizeToGrid(const std::string& name, std::int64_t sample);
    EngineHost& host_;
};

}
