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

class PortNames {
public:
    virtual ~PortNames() = default;
    virtual std::string inletName(int index) const = 0;
    virtual std::string outletName(int index) const = 0;

    static std::string channelName(const std::string& group, int channel, int width) {
        if (width == 2) return group + (channel == 0 ? " L" : " R");
        if (width <= 1) return group;
        return group + " " + std::to_string(channel + 1);
    }

    static std::string fromGroups(const std::string& groups, int index, int width) {
        const int w = width > 0 ? width : 1;
        size_t start = 0;
        while (index >= 0 && start <= groups.size()) {
            const auto comma = groups.find(',', start);
            const auto end = comma == std::string::npos ? groups.size() : comma;
            const auto first = groups.find_first_not_of(' ', start);
            auto last = groups.find_last_not_of(' ', end > 0 ? end - 1 : 0);
            if (first == std::string::npos || first >= end || last == std::string::npos || last < first) return {};
            auto name = groups.substr(first, last - first + 1);
            if (name.back() == '*') {
                name = name.substr(0, name.find_last_not_of(" *") + 1);
                return channelName(name + " " + std::to_string(index / w + 1), index % w, w);
            }
            if (index < w) return channelName(name, index, w);
            index -= w;
            if (comma == std::string::npos) return {};
            start = comma + 1;
        }
        return {};
    }
};

class PinKinds {
public:
    virtual ~PinKinds() = default;
    virtual bool controlOutlet(int valueIndex) const = 0;
    virtual bool controlInlet(const std::string&) const { return true; }
};

class NamedInlet {
public:
    static constexpr int kMaxNames = 8;
    static constexpr int kNameChars = 16;
    virtual ~NamedInlet() = default;
    virtual const char* namedInletParam() const = 0;
    virtual bool readsName(const char* name) const = 0;
    virtual const char* knobForName(const char* name) const = 0;
    virtual void setInletNames(const char (*names)[kNameChars], int count) = 0;
    virtual void setInletValue(int slot, float value) = 0;
    virtual int inletNames(char (*out)[kNameChars], int capacity) const = 0;
    virtual float inletValue(int slot) const = 0;
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
