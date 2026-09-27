// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <string>

namespace hum {

class FilePlayback {
public:
    virtual ~FilePlayback() = default;

    virtual std::int64_t playbackPosition(const std::string& name) = 0;
    virtual std::int64_t playbackLength(const std::string& name) = 0;
    virtual double playbackSampleRate(const std::string& name) = 0;
    virtual void seek(const std::string& name, std::int64_t sample) = 0;
    virtual bool ensureAudio() = 0;
};

}
