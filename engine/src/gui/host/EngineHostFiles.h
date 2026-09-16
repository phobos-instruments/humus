// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <string>

#include "gui/host/BrickHost.h"
#include "gui/host/HostCore.h"

namespace hum {


class FileHost {
public:
    FileHost(BrickHost& host, HostCore& core) : host_(host), doc_(core), audio_(core), nodes_(core), recording_(core) {}

    std::int64_t playbackPosition(const std::string& name);
    std::int64_t playbackLength(const std::string& name);
    double       playbackSampleRate(const std::string& name);
    void         seek(const std::string& name, std::int64_t sample);

    void setRecorderActive(const std::string& name, bool on);
    bool isRecorderActive(const std::string& name);
    void pollRecorders();
    void liveLooperTracks(const std::string& name, int& recording, int& armed);

private:
    BrickHost& host_;
    HostDocument& doc_;
    HostGraph& audio_;
    HostNodes& nodes_;
    HostRecording& recording_;
};

}
