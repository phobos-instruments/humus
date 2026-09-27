// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "MidiFilter/MidiFilter.h"

#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {
constexpr int kNameIn = 0, kNameLow = 1, kNameHigh = 2, kNameDo = 3, kNameTarget = 4;
constexpr unsigned char kNoteOff = 0x80, kNoteOn = 0x90, kPolyPressure = 0xA0, kControl = 0xB0;

int asInt(double v, int lo, int hi) { return std::clamp((int) std::lround(v), lo, hi); }
}

MidiFilter::MidiFilter() {
    for (int r = 0; r < kRules; ++r) {
        const auto n = std::to_string(r + 1);
        names_[(size_t) r] = {"In" + n, "Low" + n, "High" + n, "Do" + n, "Target" + n};
    }
    reset();
}

void MidiFilter::reset() {
    stagedCount_ = 0;
    outCount_ = 0;
    for (auto& ch : held_) ch.fill(Route{});
    for (auto& ch : ccNote_) ch.fill(-1);
}

void MidiFilter::readRules() {
    for (int r = 0; r < kRules; ++r) {
        const auto& n = names_[(size_t) r];
        auto& rule = rules_[(size_t) r];
        rule.in = asInt(params.get(n[kNameIn], kInOff), kInOff, kInCC);
        const int a = asInt(params.get(n[kNameLow], 60.0), 0, kMidiMax);
        const int b = asInt(params.get(n[kNameHigh], 60.0), 0, kMidiMax);
        rule.low = std::min(a, b);
        rule.high = std::max(a, b);
        rule.action = asInt(params.get(n[kNameDo], kDoBlock), kDoBlock, kDoCC);
        rule.target = asInt(params.get(n[kNameTarget], 60.0), 0, kMidiMax);
    }
    others_ = asInt(params.get("Others", kOthersPass), kOthersPass, kOthersBlock);
}

MidiFilter::Route MidiFilter::routeFor(int in, int number) const {
    for (const auto& rule : rules_) {
        if (rule.in != in || number < rule.low || number > rule.high) continue;
        if (rule.action == kDoBlock) return {Route::kBlock, 0};
        const int mapped = rule.target + (number - rule.low);
        if (mapped < 0 || mapped > kMidiMax) return {Route::kBlock, 0};
        return {rule.action == kDoNote ? Route::kNote : Route::kCC, (unsigned char) mapped};
    }
    return {others_ == kOthersBlock ? Route::kBlock : Route::kPass, (unsigned char) number};
}

void MidiFilter::emit(const MidiEvent& like, unsigned char status, int d1, int d2) {
    if (outCount_ >= (int) outEvents_.size()) return;
    MidiEvent e;
    e.sampleOffset = like.sampleOffset;
    e.data[0] = status;
    e.data[1] = (unsigned char) std::clamp(d1, 0, kMidiMax);
    e.data[2] = (unsigned char) std::clamp(d2, 0, kMidiMax);
    e.size = 3;
    outEvents_[(size_t) outCount_++] = e;
}

void MidiFilter::pass(const MidiEvent& e) {
    if (outCount_ < (int) outEvents_.size()) outEvents_[(size_t) outCount_++] = e;
}

void MidiFilter::handleNote(const MidiEvent& e, int channel, int note, int velocity, bool on) {
    auto& slot = held_[(size_t) channel][(size_t) note];
    const Route route = on || slot.kind == Route::kNone ? routeFor(kInNote, note) : slot;
    slot = on ? route : Route{};
    const auto ch = (unsigned char) channel;
    switch (route.kind) {
        case Route::kPass: pass(e); break;
        case Route::kNote: emit(e, (unsigned char) ((on ? kNoteOn : kNoteOff) | ch), route.number, velocity); break;
        case Route::kCC: emit(e, (unsigned char) (kControl | ch), route.number, on ? velocity : 0); break;
        default: break;
    }
}

void MidiFilter::handleCC(const MidiEvent& e, int channel, int controller, int value) {
    const Route route = routeFor(kInCC, controller);
    const auto ch = (unsigned char) channel;
    auto& sounding = ccNote_[(size_t) channel][(size_t) controller];
    if (route.kind != Route::kNote && sounding >= 0) {
        emit(e, (unsigned char) (kNoteOff | ch), sounding, 0);
        sounding = -1;
    }
    switch (route.kind) {
        case Route::kPass: pass(e); break;
        case Route::kCC: emit(e, (unsigned char) (kControl | ch), route.number, value); break;
        case Route::kNote: {
            const bool down = value >= kCCNoteThreshold;
            if (down && sounding < 0) {
                emit(e, (unsigned char) (kNoteOn | ch), route.number, std::max(1, value));
                sounding = route.number;
            } else if (!down && sounding >= 0) {
                emit(e, (unsigned char) (kNoteOff | ch), sounding, 0);
                sounding = -1;
            }
            break;
        }
        default: break;
    }
}

void MidiFilter::handle(const MidiEvent& e) {
    if (e.size < 1) return;
    const unsigned char kind = e.data[0] & 0xF0;
    const int channel = e.data[0] & 0x0F;
    const int d1 = e.data[1] & kMidiMax;
    const int d2 = e.data[2] & kMidiMax;
    if (e.size >= 3 && (kind == kNoteOn || kind == kNoteOff)) {
        handleNote(e, channel, d1, d2, kind == kNoteOn && d2 > 0);
        return;
    }
    if (e.size >= 3 && kind == kControl) {
        handleCC(e, channel, d1, d2);
        return;
    }
    if (e.size >= 3 && kind == kPolyPressure) {
        const Route route = routeFor(kInNote, d1);
        if (route.kind == Route::kPass) pass(e);
        else if (route.kind == Route::kNote) emit(e, e.data[0], route.number, d2);
        return;
    }
    if (others_ != kOthersBlock) pass(e);
}

void MidiFilter::process(const float* const*, int, float* const*, int, int, const Transport&) {
    readRules();
    outCount_ = 0;
    for (int i = 0; i < stagedCount_; ++i) handle(staged_[(size_t) i]);
    stagedCount_ = 0;
}

}
