#pragma once
#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace hum {

inline constexpr int kMidiSourceCount = 256;
inline constexpr int kNoteSourceBase = 128;
inline constexpr int kHeldThreshold = 63;
inline bool isMidiSource(int id) { return id >= 0 && id < kMidiSourceCount; }
inline bool isNoteSource(int id) { return id >= kNoteSourceBase && id < kMidiSourceCount; }
inline int noteOfSource(int id) { return id - kNoteSourceBase; }
inline int sourceForNote(int note) { return kNoteSourceBase + note; }
inline std::string midiSourceLabel(int id) {
    return isNoteSource(id) ? "Note " + std::to_string(noteOfSource(id))
                            : "CC " + std::to_string(id);
}

inline std::vector<int> normalizedHeld(std::vector<int> held, int cc) {
    held.erase(std::remove_if(held.begin(), held.end(),
                              [cc](int h) { return h == cc || !isMidiSource(h); }),
               held.end());
    std::sort(held.begin(), held.end());
    held.erase(std::unique(held.begin(), held.end()), held.end());
    return held;
}

struct MidiSource {
    int cc = 0;
    std::vector<int> held;

    MidiSource() = default;
    MidiSource(int c, std::vector<int> h = {}) : cc(c), held(normalizedHeld(std::move(h), c)) {}

    bool isChord() const { return !held.empty(); }
    bool operator==(const MidiSource& o) const { return cc == o.cc && held == o.held; }
    bool operator!=(const MidiSource& o) const { return !(*this == o); }
};

inline std::string midiSourceLabel(const MidiSource& s) {
    std::string out;
    for (int h : s.held) out += midiSourceLabel(h) + " + ";
    return out + midiSourceLabel(s.cc);
}

}
