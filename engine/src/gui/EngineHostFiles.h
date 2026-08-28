#pragma once
#include <cstdint>
#include <string>

namespace hum {

class EngineHost;

class FileHost {
public:
    explicit FileHost(EngineHost& host) : host_(host) {}

    std::int64_t playbackPosition(const std::string& name);
    std::int64_t playbackLength(const std::string& name);
    double       playbackSampleRate(const std::string& name);
    void         seek(const std::string& name, std::int64_t sample);

    void setRecorderActive(const std::string& name, bool on);
    bool isRecorderActive(const std::string& name);
    void pollRecorders();
    void liveLooperTracks(const std::string& name, int& recording, int& armed);

private:
    EngineHost& host_;
};

}
