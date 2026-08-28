#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

#include "hum/PatternMatrix.h"
#include "io/PatchDocument.h"

namespace hum {

enum class LooperState { Empty, Rec, Play, Dub };

inline LooperState looperState(bool armed, bool takeOpen, bool hasNotes) {
    if (armed) return takeOpen ? LooperState::Rec : LooperState::Dub;
    return hasNotes ? LooperState::Play : LooperState::Empty;
}

inline int looperCloseBars(double elapsedBeats, int beatsPerBar) {
    return std::max(1, (int) std::lround(elapsedBeats / std::max(1, beatsPerBar)));
}

inline std::vector<NoteEvent> wrapNotesIntoLoop(std::vector<NoteEvent> notes, int loopTicks) {
    if (loopTicks <= 0) return notes;
    for (auto& n : notes) {
        n.tick = ((n.tick % loopTicks) + loopTicks) % loopTicks;
        n.lengthTicks = std::min(n.lengthTicks, loopTicks);
    }
    std::sort(notes.begin(), notes.end(),
              [](const NoteEvent& a, const NoteEvent& b) { return a.tick < b.tick; });
    return notes;
}

inline bool captureAccepts(int receiveMode, int receiveChannel, unsigned char status) {
    if (receiveMode == OrganismModel::kMidiChannel)
        return ((status & 0x0F) + 1) == receiveChannel;
    return true;
}

}
