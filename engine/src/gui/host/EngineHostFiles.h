// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <string>

#include "gui/host/FileEdits.h"
#include "gui/host/BrickHost.h"
#include "gui/host/FilePlayback.h"
#include "gui/host/HostCore.h"

namespace hum {

class FileHost : public FileEdits {
public:
    FileHost(BrickHost& host, HostCore& core) : host_(host), doc_(core), audio_(core), nodes_(core), recording_(core) {}

    std::int64_t playbackPosition(const std::string& name) override;
    std::int64_t playbackLength(const std::string& name) override;
    double       playbackSampleRate(const std::string& name) override;
    void         seek(const std::string& name, std::int64_t sample) override;
    bool         ensureAudio() override { return host_.ensureAudio(); }

    bool fireTrigger(const std::string& name, const std::string& param, double value);

    void setRecorderActive(const std::string& name, bool on) override;
    bool isRecorderActive(const std::string& name) override;
    void pollRecorders();
    void liveLooperTracks(const std::string& name, int& recording, int& armed) override;

private:
    BrickHost& host_;
    HostDocument& doc_;
    HostGraph& audio_;
    HostNodes& nodes_;
    HostRecording& recording_;
};

}
