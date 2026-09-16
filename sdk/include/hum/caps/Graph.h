// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <algorithm>
#include <string>

namespace hum {

class WantsNullInlets {
public:
    virtual ~WantsNullInlets() = default;
};

class PinKinds {
public:
    virtual ~PinKinds() = default;
    virtual bool controlOutlet(int valueIndex) const = 0;
    virtual bool controlInlet(const std::string&) const { return true; }
};

class Tagged {
public:
    virtual ~Tagged() = default;
    virtual std::string tag() const = 0;
};

class TextSource {
public:
    virtual ~TextSource() = default;
    virtual int textLines(std::string* out, int capacity) const = 0;
};

class ControlSource {
public:
    struct ControlVal {
        const char* name;
        float value;
        float lo = 0.0f;
        float hi = 0.0f;
    };

    static float unitOf(const ControlVal& v) {
        if (v.hi <= v.lo) return v.value;
        return std::min(1.0f, std::max(0.0f, (v.value - v.lo) / (v.hi - v.lo)));
    }

    virtual ~ControlSource() = default;
    virtual int controlValues(ControlVal* out, int capacity) const = 0;
};

}
