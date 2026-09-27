// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include "gui/host/FilePlayback.h"

namespace hum {

class FileEdits : public FilePlayback {
public:
    ~FileEdits() override = default;

    virtual void setRecorderActive(const std::string& name, bool on) = 0;
    virtual bool isRecorderActive(const std::string& name) = 0;
    virtual void liveLooperTracks(const std::string& name, int& recording, int& armed) = 0;
};

}
