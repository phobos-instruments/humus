// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/midi/RiffImport.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include <juce_audio_basics/juce_audio_basics.h>

#include "hum/dsp/DspMath.h"

namespace hum::riff {

namespace {

constexpr int kStepsPerBeat = 4;
constexpr int kBeatsPerBar = 4;
constexpr int kStepsPerBar = kStepsPerBeat * kBeatsPerBar;
constexpr int kMinAccentSpread = 16;
constexpr int kLoudOnItsOwn = 110;
constexpr int kOctave = 12;

struct Played {
    double on = 0.0, off = 0.0;
    int note = 0, velocity = 0;
};

std::vector<Played> busiestTrack(const juce::MidiFile& file) {
    std::vector<Played> best;
    for (int t = 0; t < file.getNumTracks(); ++t) {
        juce::MidiMessageSequence seq(*file.getTrack(t));
        seq.updateMatchedPairs();
        std::vector<Played> notes;
        for (int i = 0; i < seq.getNumEvents(); ++i) {
            const auto* e = seq.getEventPointer(i);
            if (!e->message.isNoteOn()) continue;
            const double on = e->message.getTimeStamp();
            const double off = e->noteOffObject != nullptr
                                   ? e->noteOffObject->message.getTimeStamp() : on;
            notes.push_back({on, std::max(on, off), e->message.getNoteNumber(),
                             e->message.getVelocity()});
        }
        if (notes.size() > best.size()) best = std::move(notes);
    }
    std::sort(best.begin(), best.end(), [](const Played& a, const Played& b) { return a.on < b.on; });
    return best;
}

int octaveShiftToFit(const std::vector<Played>& notes) {
    int lo = kMidiMax, hi = 0;
    for (const auto& n : notes) { lo = std::min(lo, n.note); hi = std::max(hi, n.note); }
    int shift = 0;
    while (lo + shift < kBasslineLowNote && hi + shift + kOctave <= kBasslineHighNote) shift += kOctave;
    while (hi + shift > kBasslineHighNote && lo + shift - kOctave >= kBasslineLowNote) shift -= kOctave;
    return shift;
}

std::vector<BasslineStep> lay(const std::vector<Played>& notes, double stepTicks, double origin,
                              int length) {
    std::vector<BasslineStep> steps((size_t) length);
    std::vector<bool> taken((size_t) length, false);
    int lo = kMidiMax, hi = 0;
    for (const auto& n : notes) { lo = std::min(lo, n.velocity); hi = std::max(hi, n.velocity); }
    const int accentFrom = hi - lo >= kMinAccentSpread ? (lo + hi + 1) / 2 : kLoudOnItsOwn;
    const int shift = octaveShiftToFit(notes);

    for (size_t i = 0; i < notes.size(); ++i) {
        const Played& n = notes[i];
        const int at = (int) std::lround((n.on - origin) / stepTicks);
        if (at < 0 || at >= length || taken[(size_t) at]) continue;
        int next = length;
        for (size_t j = i + 1; j < notes.size(); ++j) {
            const int k = (int) std::lround((notes[j].on - origin) / stepTicks);
            if (k > at) { next = std::min(next, k); break; }
        }
        const bool overlaps = i + 1 < notes.size() && n.off > notes[i + 1].on;
        const int span = std::clamp((int) std::lround((n.off - n.on) / stepTicks), 1, next - at);

        BasslineStep step;
        step.gate = true;
        step.note = std::clamp(n.note + shift, kBasslineLowNote, kBasslineHighNote);
        step.accent = n.velocity >= accentFrom;
        for (int k = at; k < at + span; ++k) {
            steps[(size_t) k] = step;
            steps[(size_t) k].slide = k + 1 < at + span;
            taken[(size_t) k] = true;
        }
        steps[(size_t) (at + span - 1)].slide = overlaps && at + span == next;
    }
    return steps;
}

bool sameStep(const BasslineStep& a, const BasslineStep& b) {
    return a.gate == b.gate
           && (!a.gate || (a.note == b.note && a.accent == b.accent && a.slide == b.slide));
}

int repeatsEvery(const std::vector<BasslineStep>& steps, int unit) {
    const int n = (int) steps.size();
    for (int period = unit; period < n; period += unit) {
        bool holds = true;
        for (int i = period; i < n && holds; ++i)
            holds = sameStep(steps[(size_t) i], steps[(size_t) (i % period)]);
        if (holds) return period;
    }
    return n;
}

std::vector<BasslineStep> oneCycle(const std::vector<BasslineStep>& all) {
    const int cycle = repeatsEvery(all, 1);
    if (cycle <= kStepsPerBar && kStepsPerBar % cycle != 0)
        return std::vector<BasslineStep>(all.begin(), all.begin() + cycle);
    const int bars = repeatsEvery(all, kStepsPerBar);
    return std::vector<BasslineStep>(all.begin(), all.begin() + bars);
}

}

std::vector<Imported> importMidi(const std::uint8_t* data, std::size_t size) {
    std::vector<Imported> out;
    juce::MemoryInputStream in(data, size, false);
    juce::MidiFile file;
    if (!file.readFrom(in)) return out;
    const short format = file.getTimeFormat();
    if (format <= 0) return out;
    const std::vector<Played> notes = busiestTrack(file);
    if (notes.empty()) return out;

    const double stepTicks = (double) format / kStepsPerBeat;
    const double barTicks = stepTicks * kStepsPerBar;
    const double origin = std::floor(notes.front().on / barTicks) * barTicks;
    long lastStep = 0;
    for (const auto& n : notes) lastStep = std::max(lastStep, std::lround((n.on - origin) / stepTicks));
    const int bars = (int) std::clamp(lastStep / kStepsPerBar + 1, 1L, (long) kPatternBanks);
    const int length = bars * kStepsPerBar;

    const std::vector<BasslineStep> kept = oneCycle(lay(notes, stepTicks, origin, length));
    const int count = (int) kept.size();
    if (count <= kMaxWholePattern) {
        out.push_back({kept, false});
        return out;
    }
    for (int from = 0; from < count; from += kStepsPerBar)
        out.push_back({std::vector<BasslineStep>(kept.begin() + from,
                                                 kept.begin() + std::min(count, from + kStepsPerBar)),
                       false});
    return out;
}

}
