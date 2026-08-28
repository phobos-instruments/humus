#include "PianoRoll/PianoRoll.h"

#include "hum/NoteSchedule.h"

#include <algorithm>
#include <cmath>

#include "hum/Swing.h"

namespace hum {

void PianoRoll::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    held_.fill(false);
    outCount_ = 0;
    wasPlaying_ = false;
}

void PianoRoll::reset() {
    held_.fill(false);
    outCount_ = 0;
}

void PianoRoll::loadFrom(const OrganismState& state) {
    Organism::loadFrom(state);
    setPattern(state.pattern);
}

void PianoRoll::refreshClips() {
    if (!clipsDirty_) return;
    clips_ = noteschedule::prepare(pattern_, durationTicks());
    clipsDirty_ = false;
}

int PianoRoll::durationTicks() const {
    if (pattern_.present && pattern_.duration > 0) return pattern_.duration;
    const int bars = std::max(1, (int) params.get("Bars", 4.0));
    return bars * 4 * Pattern::kTicksPerBeat;
}

void PianoRoll::emit(int offset, bool on, int pitch, int velocity) {
    if (outCount_ >= (int) outEvents_.size()) return;
    const int ch = std::clamp((int) params.get("Channel", 1.0), 1, 16) - 1;
    MidiEvent e;
    e.sampleOffset = offset;
    e.data[0] = (unsigned char) ((on ? 0x90 : 0x80) | ch);
    e.data[1] = (unsigned char) std::clamp(pitch, 0, 127);
    e.data[2] = (unsigned char) (on ? std::clamp(velocity, 1, 127) : 0);
    e.size = 3;
    outEvents_[(size_t) outCount_++] = e;
    if (pitch >= 0 && pitch < 128) held_[(size_t) pitch] = on;
}

void PianoRoll::emitCC(int offset, int controller, int value) {
    if (outCount_ >= (int) outEvents_.size()) return;
    const int ch = std::clamp((int) params.get("Channel", 1.0), 1, 16) - 1;
    MidiEvent e;
    e.sampleOffset = offset;
    e.data[0] = (unsigned char) (0xB0 | ch);
    e.data[1] = (unsigned char) std::clamp(controller, 0, 127);
    e.data[2] = (unsigned char) std::clamp(value, 0, 127);
    e.size = 3;
    outEvents_[(size_t) outCount_++] = e;
}

void PianoRoll::flushHeldNotes(int offset) {
    for (int p = 0; p < 128; ++p)
        if (held_[(size_t) p]) emit(offset, false, p, 0);
}

void PianoRoll::process(const float* const*, int, float* const*, int,
                        int numSamples, const Transport& transport) {
    outCount_ = 0;
    refreshClips();

    const bool muted = params.get("Mute", 0.0) >= 0.5;
    const bool playing = transport.playing();

    if ((!playing && wasPlaying_) || (muted && !wasMuted_) || patternEdited_)
        flushHeldNotes(0);
    patternEdited_ = false;
    wasPlaying_ = playing;
    wasMuted_ = muted;
    if (!playing || muted || clips_.empty()) return;

    const double velScale = params.get("VelocityScale", 1.0);
    std::array<noteschedule::Edge, MidiNode::kMaxMidiEventsPerBlock> edges;
    const bool lp = transport.loopEnabled();
    const auto groove = params.get("SwingFollow", 1.0) >= 0.5
                            ? transport.groove()
                            : swing::grooveFor(params.get("Swing", 0.0),
                                               params.get("SwingUnit", 1.0) < 0.5
                                                   ? "1/8" : "1/16");
    const int nEdges = noteschedule::collect(clips_, transport.beats(), numSamples,
                                             transport.samplesPerBeat(), velScale,
                                             edges.data(), (int) edges.size(),
                                             lp ? transport.loopStartBeat() : 0.0,
                                             lp ? transport.loopEndBeat() : 0.0,
                                             groove);

    noteschedule::order(edges.data(), nEdges);
    for (int i = 0; i < nEdges; ++i) {
        const auto& e = edges[(size_t) i];
        if (e.cc >= 0) { emitCC(e.offset, e.cc, e.vel); continue; }
        if (!e.on && !(e.pitch >= 0 && e.pitch < 128 && held_[(size_t) e.pitch]))
            continue;
        emit(e.offset, e.on, e.pitch, e.vel);
    }
}

}
