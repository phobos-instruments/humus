// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

namespace hum {

class OscValueSource {
public:
    struct OscVal { const char* suffix; float value; };
    virtual ~OscValueSource() = default;
    virtual bool oscEnabled() const = 0;
    virtual int oscValues(OscVal* out, int capacity) const = 0;
};

class OscLogSource {
public:
    struct Logged {
        bool out = false;
        char address[72] = {};
        char args[40] = {};
    };
    virtual ~OscLogSource() = default;
    virtual void pushOsc(bool out, const char* address, const char* args) = 0;
    virtual int consumeOscLog(Logged* dest, int maxEvents) = 0;
    virtual unsigned oscLogGeneration() const = 0;
};

}
