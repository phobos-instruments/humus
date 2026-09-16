// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Ph/Ph.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>

#include "hum/dsp/DspMath.h"

namespace hum {

void Ph::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    reset();
    loadFromFile(params.getText("File"));
    fileVoice_.store((int) voiceOf(params.getText("File")), std::memory_order_relaxed);
}

void Ph::reset() {
    six_.reset();
    chip_.allOff();
    opm_.allOff();
    opl_.allOff();
    ring_.reset();
    bend_.reset();
    applyBend();
}

void Ph::applyBend() {
    const double semitones = bend_.semitones(bendRangeOf(params));
    six_.setBend(semitones);
    chip_.bend(semitones);
    opm_.bend(semitones);
    opl_.bend(semitones);
}

namespace {

bool endsWithCI(const std::string& s, const char* ext) {
    const size_t n = std::strlen(ext);
    if (s.size() < n) return false;
    for (size_t i = 0; i < n; ++i)
        if (std::tolower((unsigned char) s[s.size() - n + i]) != (unsigned char) ext[i])
            return false;
    return true;
}

}

Ph::Voice Ph::voiceOf(const std::string& bankRef) {
    if (bankRef == kFactoryChip) return Voice::Chip;
    if (bankRef == kFactoryOpm || bankRef == kFactoryOpp) return Voice::Opm;
    if (bankRef == kFactoryOpl) return Voice::Opl;
    if (bankRef.empty() || bankRef == kFactorySixOp) return Voice::SixOp;
    if (endsWithCI(bankRef, ".syx")) return Voice::SixOp;
    if (endsWithCI(bankRef, ".opm")) return Voice::Opm;
    if (endsWithCI(bankRef, ".wopl")) return Voice::Opl;
    if (endsWithCI(bankRef, ".wopn") || endsWithCI(bankRef, ".tfi")
        || endsWithCI(bankRef, ".dmp"))
        return Voice::Chip;
    return Voice::SixOp;
}

int Ph::patchCount() const {
    switch (voiceOf(params.getText("File"))) {
        case Voice::Chip: return chip_.patchCount();
        case Voice::Opm: return opm_.patchCount();
        case Voice::Opl: return opl_.patchCount();
        case Voice::SixOp: break;
    }
    return PhSixOp::kSlots;
}

std::string Ph::patchNameAt(int index) const {
    switch (voiceOf(params.getText("File"))) {
        case Voice::Chip: return chip_.patchName(index);
        case Voice::Opm: return opm_.patchName(index);
        case Voice::Opl: return opl_.patchName(index);
        case Voice::SixOp: break;
    }
    return six_.patchName(index);
}

void Ph::loadFromFile(const std::string& uri) {
    if (uri == loadedUri_) return;
    loadedUri_ = uri;
    const juce::ScopedLock sl(ioLock_);
    patchSlot_ = -1;
    if (uri.empty() || uri == kFactorySixOp) { six_.useFactoryBank(); return; }
    if (uri == kFactoryChip) { chip_.useFactoryBank(); return; }
    if (uri == kFactoryOpm || uri == kFactoryOpp) {
        opm_.setVariant(uri == kFactoryOpp);
        opm_.useFactoryBank();
        return;
    }
    if (uri == kFactoryOpl) { opl_.useFactoryBank(); return; }
    if (endsWithCI(uri, ".syx")) six_.loadSyx(uri);
    else if (endsWithCI(uri, ".opm")) { opm_.setVariant(false); opm_.loadOpm(uri); }
    else if (endsWithCI(uri, ".wopl")) opl_.loadWopl(uri);
    else if (endsWithCI(uri, ".wopn")) chip_.loadWopn(uri);
    else if (endsWithCI(uri, ".tfi") || endsWithCI(uri, ".dmp")) chip_.loadTfi(uri);
}

namespace {
std::string opParam(int op, const char* field) {
    return "Op" + std::to_string(op) + "_" + field;
}
}

std::array<Ph::OpParams, 6> Ph::makeOpParams() {
    std::array<OpParams, 6> out;
    for (int i = 1; i <= 6; ++i) {
        auto ref = [i](const char* field) { return ParamRef(opParam(i, field)); };
        out[(size_t) (i - 1)] = {ref("On"),     ref("Level"), ref("Ratio"),   ref("Detune"),
                                 ref("Attack"), ref("Decay"), ref("Sustain"), ref("Release")};
    }
    return out;
}

FmVoice Ph::readVoice(const FmVoice& fromBank) const {
    FmVoice v;
    v.algorithm = (int) algorithmRef_.get(params, (double) fromBank.algorithm);
    v.feedback = (int) feedbackRef_.get(params, (double) fromBank.feedback);
    for (int i = 1; i <= 6; ++i) {
        const FmVoiceOp& b = fromBank.ops[(size_t) (i - 1)];
        FmVoiceOp& op = v.ops[(size_t) (i - 1)];
        const auto& r = opParams_[(size_t) (i - 1)];
        const bool on = r.on.on(params, 1.0);
        op.level = on ? (int) r.level.get(params, (double) b.level) : 0;
        op.ratio = (int) r.ratio.get(params, (double) b.ratio);
        op.detune = (int) r.detune.get(params, (double) b.detune);
        op.attack = (int) r.attack.get(params, (double) b.attack);
        op.decay = (int) r.decay.get(params, (double) b.decay);
        op.sustain = (int) r.sustain.get(params, (double) b.sustain);
        op.release = (int) r.release.get(params, (double) b.release);
    }
    return v;
}

bool Ph::voiceParams(std::vector<std::pair<std::string, double>>& out) const {
    const int slot = (int) params.get("Patch", 1.0) - 1;
    const Voice voice = voiceOf(params.getText("File"));
    const FmVoice v = voice == Voice::Chip ? chip_.voiceAt(slot)
                    : voice == Voice::Opm  ? opm_.voiceAt(slot)
                    : voice == Voice::Opl  ? opl_.voiceAt(slot)
                                           : six_.voiceAt(slot);
    out.clear();
    out.emplace_back("Algorithm", (double) v.algorithm);
    out.emplace_back("Feedback", (double) v.feedback);
    for (int i = 1; i <= 6; ++i) {
        const FmVoiceOp& op = v.ops[(size_t) (i - 1)];
        out.emplace_back(opParam(i, "Level"), (double) op.level);
        out.emplace_back(opParam(i, "Ratio"), (double) op.ratio);
        out.emplace_back(opParam(i, "Detune"), (double) op.detune);
        out.emplace_back(opParam(i, "Attack"), (double) op.attack);
        out.emplace_back(opParam(i, "Decay"), (double) op.decay);
        out.emplace_back(opParam(i, "Sustain"), (double) op.sustain);
        out.emplace_back(opParam(i, "Release"), (double) op.release);
    }
    return true;
}

FmMods Ph::readMods() const {
    FmMods m;
    m.bright = modParams_.bright.get(params, 0.5);
    m.attack = modParams_.attack.get(params, 0.5);
    m.release = modParams_.release.get(params, 0.5);
    m.detune = modParams_.detune.get(params, 0.0);
    m.vibrato = modParams_.vibrato.get(params, 0.0);
    m.speed = modParams_.speed.get(params, 0.5);
    return m;
}

void Ph::pumpSixOp() {
    float buf[PhSixOp::kBlock];
    six_.renderBlock(buf);
    for (int i = 0; i < PhSixOp::kBlock; ++i) ring_.push(buf[i], buf[i]);
}

void Ph::pumpChip() {
    constexpr int kChunk = RateRing::kChunk;
    float l[kChunk], r[kChunk];
    chip_.render(l, r, kChunk);
    for (int i = 0; i < kChunk; ++i) ring_.push(l[i], r[i]);
}

void Ph::pumpOpl() {
    constexpr int kChunk = RateRing::kChunk;
    float l[kChunk], r[kChunk];
    opl_.render(l, r, kChunk);
    for (int i = 0; i < kChunk; ++i) ring_.push(l[i], r[i]);
}

void Ph::pumpOpm() {
    constexpr int kChunk = RateRing::kChunk;
    float l[kChunk], r[kChunk];
    opm_.render(l, r, kChunk);
    for (int i = 0; i < kChunk; ++i) ring_.push(l[i], r[i]);
}

void Ph::process(const float* const*, int, float* const* out, int numOut,
                 int numSamples, const Transport& transport) {
    if (numOut == 0) return;
    float* L = out[0];
    float* R = out[numOut > 1 ? 1 : 0];

    const juce::ScopedTryLock sl(ioLock_);
    if (!sl.isLocked()) {
        std::fill(L, L + numSamples, 0.0f);
        if (R != L) std::fill(R, R + numSamples, 0.0f);
        return;
    }

    const Voice voice = (Voice) fileVoice_.load(std::memory_order_relaxed);
    if (voice != voice_) {
        voice_ = voice;
        patchSlot_ = -1;
        reset();
    }
    const int slots = std::max(1, patchCount());
    const int slot = std::clamp((int) params.get("Patch", 1.0) - 1, 0, slots - 1);
    if (slot != patchSlot_) {
        patchSlot_ = slot;
        if (voice_ == Voice::SixOp) six_.selectPatch(slot);
        else if (voice_ == Voice::Opm) opm_.selectPatch(slot);
        else if (voice_ == Voice::Opl) opl_.selectPatch(slot);
        else chip_.selectPatch(slot);
        voice_params_ = voice_ == Voice::Chip ? chip_.selectedVoice()
                      : voice_ == Voice::Opm  ? opm_.selectedVoice()
                      : voice_ == Voice::Opl  ? opl_.selectedVoice()
                                              : six_.selectedVoice();
    }
    if (const auto m = readMods(); m != mods_) {
        mods_ = m;
        six_.setMods(m);
        chip_.setMods(m);
        opm_.setMods(m);
        opl_.setMods(m);
    }
    if (const auto v = readVoice(voice_params_); v != voice_params_) {
        voice_params_ = v;
        if (voice_ == Voice::SixOp) six_.setVoice(v);
        else if (voice_ == Voice::Opm) opm_.setVoice(v);
        else if (voice_ == Voice::Opl) opl_.setVoice(v);
        else chip_.setVoice(v);
    }
    const float level = (float) params.get("Level", 0.8);
    const double transpose = std::pow(2.0, params.get("Transpose", 0.0) / 12.0
                                          + params.get("Fine", 0.0) / 1200.0);

    if (std::unique_lock<std::mutex> g(liveLock_, std::try_to_lock); g.owns_lock()) {
        for (int i = 0; i < liveCount_; ++i)
            if (stagedCount_ < (int) staged_.size())
                staged_[(size_t) stagedCount_++] = liveQ_[(size_t) i];
        liveCount_ = 0;
    }
    const auto& tuning = transport.tuning();
    for (int i = 0; i < stagedCount_; ++i) {
        const auto& e = staged_[(size_t) i];
        const int st = e.data[0] & 0xF0;
        if (bend_.apply(e)) {
            applyBend();
        } else if (st == 0x90 && e.data[2] > 0) {
            const double hz = tuning.hz((double) e.data[1]) * transpose;
            if (voice_ == Voice::SixOp) six_.noteOn(e.data[1], e.data[2], hz);
            else if (voice_ == Voice::Opm) opm_.noteOn(e.data[1], e.data[2], hz);
            else if (voice_ == Voice::Opl) opl_.noteOn(e.data[1], e.data[2], hz);
            else chip_.noteOn(e.data[1], e.data[2], hz);
        } else if (st == 0x80 || (st == 0x90 && e.data[2] == 0)) {
            if (voice_ == Voice::SixOp) six_.noteOff(e.data[1]);
            else if (voice_ == Voice::Opm) opm_.noteOff(e.data[1]);
            else if (voice_ == Voice::Opl) opl_.noteOff(e.data[1]);
            else chip_.noteOff(e.data[1]);
        }
    }
    stagedCount_ = 0;

    const double sr = sampleRate_ > 0.0 ? sampleRate_ : kDefaultSampleRate;
    const double ratio = (voice_ == Voice::SixOp ? PhSixOp::kRate
                          : voice_ == Voice::Opm  ? PhOpm::kRate
                          : voice_ == Voice::Opl  ? PhOpl::kRate
                                                  : FmChip::kRate) / sr;
    while (ring_.needsMore(numSamples, ratio)) {
        if (voice_ == Voice::SixOp) pumpSixOp();
        else if (voice_ == Voice::Opm) pumpOpm();
        else if (voice_ == Voice::Opl) pumpOpl();
        else pumpChip();
    }
    ring_.read(L, R, numSamples, ratio, level);
}

}
