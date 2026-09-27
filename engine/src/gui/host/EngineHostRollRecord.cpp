// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHostMidiControl.h"

#include "core/graph/AudioGraph.h"
#include "gui/pianoroll/RollModel.h"
#include "gui/tracks/LooperFlow.h"

namespace hum {

namespace {
constexpr int kBeatsPerLoopBar = 4;
constexpr int kOpenTakeBars = 64;
constexpr int kClearedLoopBars = 4;
constexpr int kRollClip = 0;
}

int MidiHost::rollClip(const std::string& name) {
    return host_.clips().list(name).empty() ? -1 : kRollClip;
}

std::vector<NoteEvent> MidiHost::rollNotes(const std::string& name) {
    return rollClip(name) < 0 ? host_.patterns().noteEvents(name) : host_.clips().notes(name, kRollClip);
}

void MidiHost::writeRoll(const std::string& name, const std::vector<NoteEvent>& notes, int durationTicks) {
    if (rollClip(name) < 0) host_.patterns().setNoteEvents(name, notes, durationTicks);
    else host_.clips().setNotes(name, kRollClip, notes, durationTicks);
}

NoteRecorder* MidiHost::recorderOf(const std::string& name) {
    auto* g = audio_.graph();
    return g == nullptr ? nullptr : dynamic_cast<NoteRecorder*>(g->find(name));
}

bool MidiHost::recorderSwitch(const std::string& name, const std::string& param, double value) {
    auto* rec = recorderOf(name);
    if (rec == nullptr) return false;
    const bool high = value >= 0.5;
    if (param == rec->recordSwitch()) {
        recordRoll(name, high);
        return true;
    }
    if (param == rec->loopSwitch()) {
        auto& take = rolls_[name];
        const bool edge = high && !take.loopHigh;
        take.loopHigh = high;
        if (edge) tapLoop(name);
        return true;
    }
    if (param == rec->quantizeSwitch()) requantize(name, high ? rolls_[name].grid : 0);
    return false;
}

int MidiHost::recordQuantize(const std::string& name) {
    auto* rec = recorderOf(name);
    const bool on = rec == nullptr || host_.liveParamValue(name, rec->quantizeSwitch()) >= 0.5;
    return on ? rolls_[name].grid : 0;
}

void MidiHost::requantize(const std::string& name, int ticks) {
    const juce::ScopedLock ml(state_.targetsLock);
    for (auto& t : state_.recordTargets)
        if (t.node == name && !t.inlet) t.quantize = ticks;
}

void MidiHost::recordRoll(const std::string& name, bool on) {
    if (on == isRecordTarget(name)) return;
    if (!on) {
        setRecordTarget(name, false);
        rolls_[name].open = false;
        return;
    }
    host_.ensureAudio();
    host_.patterns().ensureNote(name);
    if (!host_.isPlaying()) host_.play();
    setRecordTarget(name, true, recordQuantize(name), rollClip(name), true);
}

void MidiHost::tapLoop(const std::string& name) {
    auto& take = rolls_[name];
    host_.patterns().ensureNote(name);
    const bool hasNotes = !rollNotes(name).empty();
    const int bar = kBeatsPerLoopBar * Pattern::kTicksPerBeat;
    switch (roll::loopTapFor(isRecordTarget(name), take.open, hasNotes)) {
        case roll::LoopTap::CloseTake: {
            setRecordTarget(name, false);
            take.open = false;
            const int loopTicks = looperCloseBars(host_.positionBeats(), kBeatsPerLoopBar) * bar;
            writeRoll(name, wrapNotesIntoLoop(rollNotes(name), loopTicks), loopTicks);
            break;
        }
        case roll::LoopTap::Disarm:
            setRecordTarget(name, false);
            break;
        case roll::LoopTap::StartTake:
            host_.ensureAudio();
            host_.pushUndo();
            writeRoll(name, {}, kOpenTakeBars * bar);
            host_.setPositionBeats(0.0);
            if (!host_.isPlaying()) host_.play();
            setRecordTarget(name, true, recordQuantize(name), rollClip(name), true);
            take.open = true;
            break;
        case roll::LoopTap::Overdub:
            host_.ensureAudio();
            if (!host_.isPlaying()) host_.play();
            setRecordTarget(name, true, recordQuantize(name), rollClip(name), true);
            break;
    }
}

bool MidiHost::loopTakeOpen(const std::string& name) const {
    const auto it = rolls_.find(name);
    return it != rolls_.end() && it->second.open;
}

void MidiHost::clearLoop(const std::string& name) {
    if (isRecordTarget(name)) setRecordTarget(name, false);
    rolls_[name].open = false;
    host_.pushUndo();
    writeRoll(name, {}, kClearedLoopBars * kBeatsPerLoopBar * Pattern::kTicksPerBeat);
}

void MidiHost::setRecordGrid(const std::string& name, int ticks) {
    if (ticks <= 0) return;
    rolls_[name].grid = ticks;
    if (recordQuantize(name) > 0) requantize(name, ticks);
}

}
