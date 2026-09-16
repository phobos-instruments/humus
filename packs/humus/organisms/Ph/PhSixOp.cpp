// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Ph/PhSixOp.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "exp2.h"
#include "freqlut.h"
#include "patch.h"
#include "pitchenv.h"
#include "sin.h"
#undef N

#include <juce_core/juce_core.h>

#include "Ph/PhSixOpBank.h"

namespace hum {

namespace {

void initTablesOnce() {
    static const bool done = [] {
        Sin::init();
        Exp2::init();
        Freqlut::init(PhSixOp::kRate);
        Lfo::init(PhSixOp::kRate);
        PitchEnv::init(PhSixOp::kRate);
        return true;
    }();
    juce::ignoreUnused(done);
}

constexpr uint8_t kCarriers[32] = {
    0x28, 0x28, 0x24, 0x24, 0x2a, 0x2a, 0x28, 0x28,
    0x28, 0x24, 0x24, 0x28, 0x28, 0x28, 0x28, 0x20,
    0x20, 0x20, 0x26, 0x34, 0x36, 0x2e, 0x36, 0x3e,
    0x3e, 0x34, 0x34, 0x29, 0x3a, 0x39, 0x3e, 0x3f,
};

}

PhSixOp::PhSixOp() {
    initTablesOnce();
    controllers_.values_[kControllerPitch] = 0x2000;
    useFactoryBank();
    selectPatch(0);
}

void PhSixOp::reset() {
    allOff();
    for (auto& v : voices_) v.midi = -1;
}

void PhSixOp::useFactoryBank() {
    phbank::fillSixOpFactory(reinterpret_cast<uint8_t(*)[128]>(bank_.data()), kSlots);
    currentSlot_ = -1;
}

bool PhSixOp::loadSyx(const std::string& path) {
    juce::File f(juce::String(juce::CharPointer_UTF8(path.c_str())));
    juce::MemoryBlock raw;
    if (!f.existsAsFile() || !f.loadFileAsData(raw)) return false;
    const auto* d = static_cast<const uint8_t*>(raw.getData());
    const size_t n = raw.getSize();
    size_t off = std::string::npos;
    if (n >= 4104 && d[0] == 0xF0) off = 6;
    else if (n >= 4096 && n < 4104) off = 0;
    if (off == std::string::npos) return false;
    for (int s = 0; s < kSlots; ++s)
        std::memcpy(bank_[(size_t) s].data(), d + off + (size_t) s * 128, 128);
    currentSlot_ = -1;
    return true;
}

std::string PhSixOp::patchName(int slot) const {
    if (slot < 0 || slot >= kSlots) return {};
    std::string name(reinterpret_cast<const char*>(bank_[(size_t) slot].data()) + 118, 10);
    while (!name.empty() && (name.back() == ' ' || (uint8_t) name.back() < 32))
        name.pop_back();
    for (auto& ch : name)
        if ((uint8_t) ch < 32 || (uint8_t) ch > 126) ch = '?';
    return name;
}

void PhSixOp::selectPatch(int slot) {
    slot = std::clamp(slot, 0, kSlots - 1);
    if (slot == currentSlot_) return;
    currentSlot_ = slot;
    UnpackPatch(reinterpret_cast<const char*>(bank_[(size_t) slot].data()), current_);
    hasEdit_ = false;
    rebuildVoiced();
    allOff();
}

namespace {
constexpr int kOpOffset(int playerOp) { return (6 - playerOp) * 21; }
}

FmVoice PhSixOp::voiceAt(int slot) const {
    char unpacked[156] = {0};
    const int s = std::clamp(slot, 0, kSlots - 1);
    UnpackPatch(reinterpret_cast<const char*>(bank_[(size_t) s].data()), unpacked);
    FmVoice v;
    const auto* p = reinterpret_cast<const unsigned char*>(unpacked);
    v.algorithm = (p[134] & 31) + 1;
    v.feedback = p[135] & 7;
    for (int i = 1; i <= 6; ++i) {
        const auto* o = p + kOpOffset(i);
        FmVoiceOp& op = v.ops[(size_t) (i - 1)];
        op.attack = o[0];
        op.decay = o[1];
        op.release = o[3];
        op.sustain = o[6];
        op.level = o[16];
        op.ratio = o[18];
        op.detune = o[20];
    }
    return v;
}

void PhSixOp::setVoice(const FmVoice& v) {
    std::memcpy(edited_, current_, sizeof(edited_));
    auto* p = reinterpret_cast<unsigned char*>(edited_);
    p[134] = (unsigned char) std::clamp(v.algorithm - 1, 0, 31);
    p[135] = (unsigned char) std::clamp(v.feedback, 0, 7);
    for (int i = 1; i <= 6; ++i) {
        auto* o = p + kOpOffset(i);
        const FmVoiceOp& op = v.ops[(size_t) (i - 1)];
        o[0] = (unsigned char) std::clamp(op.attack, 0, 99);
        o[1] = (unsigned char) std::clamp(op.decay, 0, 99);
        o[3] = (unsigned char) std::clamp(op.release, 0, 99);
        o[6] = (unsigned char) std::clamp(op.sustain, 0, 99);
        o[16] = (unsigned char) std::clamp(op.level, 0, 99);
        o[18] = (unsigned char) std::clamp(op.ratio, 0, 31);
        o[20] = (unsigned char) std::clamp(op.detune, 0, 14);
    }
    hasEdit_ = true;
    rebuildVoiced();
}

void PhSixOp::setMods(const FmMods& m) {
    if (m == mods_) return;
    mods_ = m;
    rebuildVoiced();
}

void PhSixOp::rebuildVoiced() {
    std::memcpy(voiced_, hasEdit_ ? edited_ : current_, sizeof(voiced_));
    const uint8_t carriers = kCarriers[(unsigned char) voiced_[134] & 31];
    const int lift = (int) std::lround((mods_.bright - 0.5) * 60.0);
    const int slowAtk = (int) std::lround((mods_.attack - 0.5) * 80.0);
    const int slowRel = (int) std::lround((mods_.release - 0.5) * 80.0);
    const int spread = (int) std::lround(mods_.detune / 50.0 * 7.0);
    for (int op = 0; op < 6; ++op) {
        auto* p = reinterpret_cast<uint8_t*>(voiced_) + op * 21;
        if (((carriers >> op) & 1) == 0)
            p[16] = (uint8_t) std::clamp((int) p[16] + lift, 0, 99);
        p[0] = (uint8_t) std::clamp((int) p[0] - slowAtk, 0, 99);
        p[3] = (uint8_t) std::clamp((int) p[3] - slowRel, 0, 99);
        if (spread != 0)
            p[20] = (uint8_t) std::clamp((int) p[20] + (op % 2 == 0 ? spread : -spread), 0, 14);
    }
    auto* v = reinterpret_cast<uint8_t*>(voiced_);
    v[137] = (uint8_t) std::clamp((int) std::lround(mods_.speed * 99.0), 0, 99);
    if (mods_.vibrato > 0.0) {
        v[139] = (uint8_t) std::clamp((int) std::lround(mods_.vibrato * 99.0), 0, 99);
        v[143] = (uint8_t) std::max<int>(v[143], 4);
    }
    lfo_.reset(voiced_ + 137);
}

void PhSixOp::setBend(double semitones) {
    controllers_.values_[kControllerPitch] = 0x2000 + (int) std::lround(semitones / 3.0 * 8192.0);
}

void PhSixOp::noteOn(int midinote, int velocity, double hz) {
    int slot = -1;
    for (int i = 0; i < kVoices; ++i)
        if (voices_[(size_t) i].midi < 0) { slot = i; break; }
    if (slot < 0) { slot = next_; next_ = (next_ + 1) % kVoices; }
    Voice& v = voices_[(size_t) slot];
    const auto logfreq = (int32_t) std::lround(16777216.0 * std::log2(std::max(1.0, hz)));
    v.note.initLogfreq(voiced_, logfreq, midinote, velocity);
    v.midi = midinote;
    v.gate = true;
    v.quiet = 0;
    lfo_.keydown();
}

void PhSixOp::noteOff(int midinote) {
    for (auto& v : voices_)
        if (v.midi == midinote && v.gate) {
            v.gate = false;
            v.note.keyup();
        }
}

void PhSixOp::allOff() {
    for (auto& v : voices_)
        if (v.midi >= 0) {
            v.note.keyup();
            v.gate = false;
        }
}

void PhSixOp::renderBlock(float* out) {
    const int32_t lfoval = lfo_.getsample();
    const int32_t lfodelay = lfo_.getdelay();
    int32_t mix[kBlock];
    std::memset(mix, 0, sizeof(mix));
    int32_t vb[kBlock];
    for (auto& v : voices_) {
        if (v.midi < 0) continue;
        std::memset(vb, 0, sizeof(vb));
        v.note.compute(vb, lfoval, lfodelay, &controllers_);
        int32_t peak = 0;
        for (int i = 0; i < kBlock; ++i) {
            mix[i] += vb[i];
            peak = std::max(peak, std::abs(vb[i]));
        }
        if (!v.gate) {
            v.quiet = peak < (1 << 11) ? v.quiet + 1 : 0;
            if (v.quiet >= 4) v.midi = -1;
        }
    }
    constexpr float kScale = 0.72f / (float) (1 << 26);
    for (int i = 0; i < kBlock; ++i) out[i] = (float) mix[i] * kScale;
}

}
