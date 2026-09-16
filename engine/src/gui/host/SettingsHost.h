// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <string>

#include "gui/host/BrickHost.h"

namespace hum {

class GamepadHost;

class SettingsHost : public virtual BrickHost {
public:
    enum MidiSync { kSyncOff = 0, kSyncGenerate = 1, kSyncChase = 2 };

    ~SettingsHost() override = default;

    virtual GamepadHost& gamepads() = 0;
    virtual int  midiSyncMode() const = 0;
    virtual void setMidiSyncMode(int mode) = 0;
    virtual void persistAudioState() = 0;
};

}
