// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "MidiPlayer/MidiPlayer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "hum/PitchBend.h"
#include "hum/Registry.h"

namespace hum {

namespace {
constexpr int kChannels = midisong::kChannels;
constexpr double kMinBpm = 1.0;
}

void MidiPlayer::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate > 0.0 ? sampleRate : kDefaultSampleRate;
    voices_.prepare(sampleRate_);
    voices_.setBank(&bank_);
    sf2_.prepare(sampleRate_);
    loadBank(params.getText("Bank"));
    loadFromFile(params.getText("File"));
    rewind();
}

void MidiPlayer::reset() {
    voices_.reset();
    sf2_.reset();
    rewind();
}

void MidiPlayer::rewind() {
    cursor_ = 0.0;
    next_ = 0;
    voiceAllOff(-1);
    posSamples_.store(0);
    for (int c = 0; c < kChannels; ++c)
        voiceProgram(c, song_.program.size() > (size_t) c ? song_.program[(size_t) c] : 0);
}

void MidiPlayer::loadBank(const std::string& uri) {
    if (uri == loadedBank_) return;
    loadedBank_ = uri;
    if (uri.empty()) return;
    const juce::File file(juce::String(juce::CharPointer_UTF8(uri.c_str())));
    if (sf2::isSf2Path(uri)) {
        voiceAllOff(-1);
        if (sf2_.load(file)) engine_ = Engine::SoundFont;
        return;
    }
    voiceAllOff(-1);
    if (bank_.load(file)) engine_ = Engine::Fm;
}

void MidiPlayer::voiceNoteOn(int channel, int note, int velocity) {
    if (engine_ == Engine::SoundFont) sf2_.noteOn(channel, note, velocity);
    else voices_.noteOn(channel, note, velocity);
}

void MidiPlayer::voiceNoteOff(int channel, int note) {
    if (engine_ == Engine::SoundFont) sf2_.noteOff(channel, note);
    else voices_.noteOff(channel, note);
}

void MidiPlayer::voiceAllOff(int channel) {
    sf2_.allNotesOff(channel);
    voices_.allNotesOff(channel);
}

void MidiPlayer::voiceProgram(int channel, int program) {
    if (engine_ == Engine::SoundFont) sf2_.programChange(channel, program);
    else voices_.programChange(channel, program);
}

void MidiPlayer::voiceBend(int channel, int wheel) {
    const double semitones = (double) (wheel - PitchBend::kCentre) / (double) PitchBend::kCentre
                             * kDefaultBendRange;
    if (engine_ == Engine::SoundFont) sf2_.pitchBend(channel, semitones);
    else voices_.pitchBend(channel, semitones);
}

void MidiPlayer::loadFromFile(const std::string& uri) {
    if (uri == loadedSong_) return;
    loadedSong_ = uri;
    auto fresh = std::make_unique<midisong::Song>();
    if (!uri.empty())
        midisong::loadSong(juce::File(juce::String(juce::CharPointer_UTF8(uri.c_str()))), *fresh);
    const juce::ScopedLock lock(swapLock_);
    pending_ = std::move(fresh);
    hasPending_.store(true);
}

void MidiPlayer::onTextChanged(const std::string& param, const std::string& text) {
    if (param == "File") loadFromFile(text);
    else if (param == "Bank") loadBank(text);
}

void MidiPlayer::adoptPending() {
    if (!hasPending_.load()) return;
    std::unique_ptr<midisong::Song> taken;
    {
        const juce::ScopedLock lock(swapLock_);
        taken = std::move(pending_);
        hasPending_.store(false);
    }
    if (taken == nullptr) return;
    song_ = std::move(*taken);
    lenSamples_.store((std::int64_t) (song_.secondsForBeats(song_.lengthBeats) * sampleRate_));
    rewind();
}

int MidiPlayer::programFor(int channel) const {
    const auto name = "Program" + std::to_string(channel + 1);
    return (int) std::lround(params.get(name, 0.0));
}

bool MidiPlayer::muted(int channel) const {
    return params.get("Mute" + std::to_string(channel + 1), 0.0) > 0.5;
}

void MidiPlayer::emitUpTo(double beat) {
    while (next_ < song_.events.size() && song_.events[next_].beat <= beat) {
        const auto& e = song_.events[next_++];
        const int ch = (int) e.channel;
        if (ch < 0 || ch >= kChannels) continue;
        switch (e.kind) {
            case midisong::Kind::NoteOn:
                if (!muted(ch)) voiceNoteOn(ch, e.a, e.b);
                break;
            case midisong::Kind::NoteOff:  voiceNoteOff(ch, e.a); break;
            case midisong::Kind::AllNotesOff: voiceAllOff(ch); break;
            case midisong::Kind::Program:
                if (programFor(ch) == 0) voiceProgram(ch, e.a);
                break;
            case midisong::Kind::PitchBend:
                voiceBend(ch, e.a | (e.b << 7));
                break;
            case midisong::Kind::Controller:
                break;
        }
    }
}

void MidiPlayer::process(const float* const*, int, float* const* out, int numOut,
                         int numSamples, const Transport& transport) {
    if (numOut == 0) return;
    adoptPending();
    loadBank(params.getText("Bank"));

    voices_.setChipCount((int) std::lround(params.get("Chips", 4.0)));
    for (int c = 0; c < kChannels; ++c)
        if (const int forced = programFor(c); forced > 0)
            voiceProgram(c, forced - 1);

    if (const auto want = seekReq_.exchange(-1); want >= 0) {
        cursor_ = song_.beatsForSeconds((double) want / sampleRate_);
        next_ = 0;
        while (next_ < song_.events.size() && song_.events[next_].beat < cursor_) ++next_;
        voiceAllOff(-1);
    }

    const bool playing = params.get("Play", 0.0) > 0.5 && !song_.events.empty();
    if (playing && !wasPlaying_ && cursor_ >= song_.lengthBeats) rewind();
    if (!playing && wasPlaying_) voiceAllOff(-1);
    wasPlaying_ = playing;

    if (playing) {
        const bool follow = params.get("FollowHost", 1.0) > 0.5;
        const double bpm = follow ? std::max(kMinBpm, transport.tempo())
                                  : std::max(kMinBpm, song_.bpmAt(cursor_));
        cursor_ += (double) numSamples / sampleRate_ * bpm / kSecondsPerMinute;
        emitUpTo(cursor_);
        if (cursor_ >= song_.lengthBeats) {
            if (params.get("Loop", 0.0) > 0.5) rewind();
            else voiceAllOff(-1);
        }
        posSamples_.store((std::int64_t) (song_.secondsForBeats(cursor_) * sampleRate_));
    }

    const auto level = (float) params.get("Level", 0.7);
    float* l = out[0];
    float* r = numOut > 1 ? out[1] : out[0];
    if (engine_ == Engine::SoundFont) sf2_.render(l, r, numSamples, level);
    else voices_.render(l, r, numSamples, level);
}

}
