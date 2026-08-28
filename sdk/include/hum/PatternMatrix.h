#pragma once
#include <cstdlib>
#include <string>
#include <vector>

#include "hum/Pattern.h"

namespace hum {

struct BasslineStep {
    bool gate = false;
    bool accent = false;
    bool slide = false;
    int note = 60;
};

struct ArpStep {
    bool trigger = false;
    bool tie = false;
};

inline int stepTicksFor(const std::string& resolution) {
    int denom = 16;
    auto slash = resolution.find('/');
    if (slash != std::string::npos) {
        denom = std::atoi(resolution.c_str() + slash + 1);
        if (denom <= 0) denom = 16;
    }
    return (Pattern::kTicksPerBeat * 4) / denom;
}

inline std::vector<BasslineStep> decodeBassline(const std::string& m) {
    std::vector<BasslineStep> out;
    for (size_t i = 0; i + 5 <= m.size(); i += 5) {
        BasslineStep s;
        s.gate = m[i] == 'x';
        s.accent = m[i + 1] == '!';
        s.slide = m[i + 2] == '-';
        const std::string hex = m.substr(i + 3, 2);
        s.note = (int) std::strtol(hex.c_str(), nullptr, 16);
        out.push_back(s);
    }
    return out;
}

inline std::string encodeBassline(const std::vector<BasslineStep>& steps) {
    std::string m;
    m.reserve(steps.size() * 5);
    static const char* hexU = "0123456789ABCDEF";
    for (const auto& s : steps) {
        m += s.gate ? 'x' : '.';
        m += s.accent ? '!' : ' ';
        m += s.slide ? '-' : ' ';
        const int n = s.note & 0xFF;
        m += hexU[(n >> 4) & 0xF];
        m += hexU[n & 0xF];
    }
    return m;
}

inline std::vector<ArpStep> decodeArp(const std::string& m) {
    std::vector<ArpStep> out;
    for (size_t i = 0; i + 2 <= m.size(); i += 2) {
        ArpStep s;
        s.trigger = m[i] == 'x';
        s.tie = m[i + 1] == '-';
        out.push_back(s);
    }
    return out;
}

inline std::string encodeArp(const std::vector<ArpStep>& steps) {
    std::string m;
    m.reserve(steps.size() * 2);
    for (const auto& s : steps) {
        m += s.trigger ? 'x' : '.';
        m += s.tie ? '-' : ' ';
    }
    return m;
}

inline std::vector<bool> decodeOctaveRow(const std::string& m) {
    std::vector<bool> out;
    out.reserve(m.size());
    for (char c : m) out.push_back(c == '^');
    return out;
}

inline std::string encodeOctaveRow(const std::vector<bool>& ups) {
    std::string m;
    m.reserve(ups.size());
    for (bool u : ups) m += u ? '^' : '.';
    return m;
}

struct NoteEvent {
    int tick = 0;
    int pitch = 60;
    int lengthTicks = 12;
    int velocity = 100;
};

inline std::vector<NoteEvent> decodeNoteEvents(const std::string& text) {
    std::vector<NoteEvent> out;
    const char* p = text.c_str();
    while (*p != '\0') {
        char* end = nullptr;
        const long tick = std::strtol(p, &end, 10);
        if (end == p) { while (*p != '\0' && *p != '\n') ++p; if (*p) ++p; continue; }
        p = end;
        const long pitch = std::strtol(p, &end, 10); if (end == p) continue; p = end;
        const long len   = std::strtol(p, &end, 10); if (end == p) continue; p = end;
        const long vel   = std::strtol(p, &end, 10); if (end == p) continue; p = end;
        NoteEvent n;
        n.tick = (int) (tick < 0 ? 0 : tick);
        n.pitch = (int) (pitch < 0 ? 0 : pitch > 127 ? 127 : pitch);
        n.lengthTicks = (int) (len < 1 ? 1 : len);
        n.velocity = (int) (vel < 1 ? 1 : vel > 127 ? 127 : vel);
        out.push_back(n);
    }
    return out;
}

inline std::string encodeNoteEvents(const std::vector<NoteEvent>& notes) {
    std::string m;
    for (const auto& n : notes)
        m += std::to_string(n.tick) + " " + std::to_string(n.pitch) + " "
           + std::to_string(n.lengthTicks) + " " + std::to_string(n.velocity) + "\n";
    return m;
}

struct CCEvent {
    int tick = 0;
    int controller = 1;
    int value = 64;
};

inline std::vector<CCEvent> decodeCCEvents(const std::string& text) {
    std::vector<CCEvent> out;
    const char* p = text.c_str();
    while (*p != '\0') {
        while (*p == ' ' || *p == '\t') ++p;
        if (*p == 'c') {
            ++p;
            char* end = nullptr;
            const long tick = std::strtol(p, &end, 10);
            if (end != p) {
                p = end;
                const long num = std::strtol(p, &end, 10);
                if (end != p) {
                    p = end;
                    const long val = std::strtol(p, &end, 10);
                    if (end != p) {
                        p = end;
                        CCEvent c;
                        c.tick = (int) (tick < 0 ? 0 : tick);
                        c.controller = (int) (num < 0 ? 0 : num > 127 ? 127 : num);
                        c.value = (int) (val < 0 ? 0 : val > 127 ? 127 : val);
                        out.push_back(c);
                    }
                }
            }
        }
        while (*p != '\0' && *p != '\n') ++p;
        if (*p) ++p;
    }
    return out;
}

inline std::string encodeCCEvents(const std::vector<CCEvent>& ccs) {
    std::string m;
    for (const auto& c : ccs)
        m += "c " + std::to_string(c.tick) + " " + std::to_string(c.controller) + " "
           + std::to_string(c.value) + "\n";
    return m;
}

inline std::string replaceNoteEvents(const std::string& matrix, const std::vector<NoteEvent>& notes) {
    return encodeNoteEvents(notes) + encodeCCEvents(decodeCCEvents(matrix));
}
inline std::string replaceCCEvents(const std::string& matrix, const std::vector<CCEvent>& ccs) {
    return encodeNoteEvents(decodeNoteEvents(matrix)) + encodeCCEvents(ccs);
}

inline const PatternChannel* matrixChannel(const Pattern& p, const std::string& type) {
    for (const auto& ch : p.channels)
        if (ch.type == type) return &ch;
    return nullptr;
}

}
