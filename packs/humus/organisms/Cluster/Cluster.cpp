#include "Cluster/Cluster.h"

#include <cctype>
#include <cstdlib>

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {
enum Mode { kModePads = 0, kModeFollow = 1 };
enum Hold { kHoldGate = 0, kHoldLatch = 1, kHoldPedal = 2 };

int semitoneOf(char letter) {
    switch (letter) {
        case 'C': return 0;
        case 'D': return 2;
        case 'E': return 4;
        case 'F': return 5;
        case 'G': return 7;
        case 'A': return 9;
        case 'B': return 11;
        default: return -1;
    }
}
}

int Cluster::parseNotes(const std::string& text, int* out, int capacity) {
    int count = 0;
    const char* p = text.c_str();
    while (*p && count < capacity) {
        while (*p && !std::isalnum((unsigned char) *p) && *p != '-') ++p;
        if (!*p) break;
        int note = -1;
        const int semi = semitoneOf((char) std::toupper((unsigned char) *p));
        if (semi >= 0 && !std::isdigit((unsigned char) *p)) {
            ++p;
            int acc = 0;
            while (*p == '#' || *p == 'b') acc += *p++ == '#' ? 1 : -1;
            char* end = nullptr;
            const long oct = std::strtol(p, &end, 10);
            note = (end != p ? ((int) oct + 1) * 12 : 48) + semi + acc;
            p = end != p ? end : p;
        } else if (std::isdigit((unsigned char) *p)) {
            char* end = nullptr;
            note = (int) std::strtol(p, &end, 10);
            p = end;
        } else {
            ++p;
        }
        if (note >= 0 && note <= kMidiMax) out[count++] = note;
    }
    return count;
}

void Cluster::reset() {
    stagedCount_ = 0;
    outCount_ = 0;
    for (auto& s : pad_) s = {};
    for (auto& s : follow_) s = {};
    followLatch_ = {};
    refs_.fill(0);
    lastFire_.fill(false);
    firePrimed_ = false;
}

void Cluster::emit(int offset, bool on, int note, int velocity) {
    if (outCount_ >= (int) outEvents_.size()) return;
    MidiEvent e;
    e.sampleOffset = offset;
    e.data[0] = on ? 0x90 : 0x80;
    e.data[1] = (unsigned char) std::clamp(note, 0, kMidiMax);
    e.data[2] = (unsigned char) std::clamp(velocity, 0, kMidiMax);
    e.size = 3;
    outEvents_[(size_t) outCount_++] = e;
}

void Cluster::soundChord(const int* notes, int count, int velocity, int offset,
                         Emitted& slot) {
    slot.count = 0;
    for (int i = 0; i < count && slot.count < kChordMax; ++i) {
        const int n = std::clamp(notes[i], 0, kMidiMax);
        if (refs_[(size_t) n]++ == 0) emit(offset, true, n, velocity);
        slot.notes[(size_t) slot.count++] = n;
    }
    slot.active = true;
}

void Cluster::hushChord(Emitted& slot, int offset) {
    for (int i = 0; i < slot.count; ++i) {
        const int n = slot.notes[(size_t) i];
        if (refs_[(size_t) n] > 0 && --refs_[(size_t) n] == 0)
            emit(offset, false, n, 0);
    }
    slot.count = 0;
    slot.active = false;
}

void Cluster::refreshSlotTexts() {
    for (int s = 0; s < kSlots; ++s) {
        const std::string text = params.getText("Notes" + std::to_string(s + 1));
        if (text == cachedText_[(size_t) s]) continue;
        cachedText_[(size_t) s] = text;
        if (pad_[(size_t) s].active) hushChord(pad_[(size_t) s], 0);
        chordCount_[(size_t) s] =
            parseNotes(text, chord_[(size_t) s].data(), kChordMax);
    }
}

void Cluster::hushAll(int offset) {
    for (auto& s : pad_) if (s.active) hushChord(s, offset);
    for (auto& s : follow_) if (s.active) hushChord(s, offset);
    if (followLatch_.active) hushChord(followLatch_, offset);
}

void Cluster::handleFireEdges(int hold) {
    const int velocity = (int) std::clamp(params.get("Velocity", 100.0), 1.0, kMidiMaxD);
    for (int s = 0; s < kSlots; ++s) {
        const bool now = params.get("Fire" + std::to_string(s + 1), 0.0) >= 0.5;
        const bool was = lastFire_[(size_t) s];
        lastFire_[(size_t) s] = now;
        if (!firePrimed_) continue;
        auto& slot = pad_[(size_t) s];
        if (now && !was) {
            if (hold == kHoldPedal) {
                hushAll(0);
                soundChord(chord_[(size_t) s].data(), chordCount_[(size_t) s],
                           velocity, 0, slot);
            } else if (hold == kHoldLatch && slot.active) {
                hushChord(slot, 0);
            } else {
                if (slot.active) hushChord(slot, 0);
                soundChord(chord_[(size_t) s].data(), chordCount_[(size_t) s],
                           velocity, 0, slot);
            }
        } else if (!now && was && hold == kHoldGate && slot.active) {
            hushChord(slot, 0);
        }
    }
    firePrimed_ = true;
}

void Cluster::handleEvent(const MidiEvent& e, int mode, int hold, int triggerNote,
                          int followSlot, int velocityParam) {
    const int status = e.data[0] & 0xF0;
    const bool isOn = status == 0x90 && e.data[2] > 0;
    const bool isOff = status == 0x80 || (status == 0x90 && e.data[2] == 0);
    if (!isOn && !isOff) return;
    const int note = e.data[1];
    const int offset = e.sampleOffset;

    if (mode == kModeFollow) {
        const int s = std::clamp(followSlot, 0, kSlots - 1);
        const int count = chordCount_[(size_t) s];
        if (count < 1) return;
        int notes[kChordMax];
        for (int i = 0; i < count; ++i)
            notes[i] = note + chord_[(size_t) s][(size_t) i] - chord_[(size_t) s][0];
        if (hold != kHoldGate) {
            if (!isOn) return;
            if (hold == kHoldPedal) hushAll(offset);
            else hushChord(followLatch_, offset);
            soundChord(notes, count, e.data[2], offset, followLatch_);
        } else if (isOn) {
            auto& slot = follow_[(size_t) note];
            if (slot.active) hushChord(slot, offset);
            soundChord(notes, count, e.data[2], offset, slot);
        } else {
            hushChord(follow_[(size_t) note], offset);
        }
        return;
    }

    const int s = note - triggerNote;
    if (s < 0 || s >= kSlots) return;
    auto& slot = pad_[(size_t) s];
    const int velocity = isOn ? e.data[2] : velocityParam;
    if (isOn) {
        if (hold == kHoldPedal) {
            hushAll(offset);
            soundChord(chord_[(size_t) s].data(), chordCount_[(size_t) s],
                       velocity, offset, slot);
        } else if (hold == kHoldLatch && slot.active) {
            hushChord(slot, offset);
        } else {
            if (slot.active) hushChord(slot, offset);
            soundChord(chord_[(size_t) s].data(), chordCount_[(size_t) s],
                       velocity, offset, slot);
        }
    } else if (hold == kHoldGate && slot.active) {
        hushChord(slot, offset);
    }
}

void Cluster::process(const float* const*, int, float* const*, int, int,
                      const Transport&) {
    const int mode = (int) std::clamp(params.get("Mode", 0.0), 0.0, 1.0);
    const int hold = (int) std::clamp(params.get("Hold", 0.0), 0.0, 2.0);
    const int triggerNote = (int) std::clamp(params.get("TriggerNote", 36.0), 0.0, 120.0);
    const int followSlot = (int) std::clamp(params.get("Slot", 1.0), 1.0, 8.0) - 1;
    const int velocity = (int) std::clamp(params.get("Velocity", 100.0), 1.0, kMidiMaxD);

    if (mode != lastMode_ || hold != lastHold_) {
        hushAll(0);
        lastMode_ = mode;
        lastHold_ = hold;
    }
    refreshSlotTexts();
    handleFireEdges(hold);
    for (int i = 0; i < stagedCount_; ++i)
        handleEvent(staged_[(size_t) i], mode, hold, triggerNote, followSlot, velocity);
    stagedCount_ = 0;
}

}
