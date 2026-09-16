// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "common/FmChip.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include <juce_core/juce_core.h>

#include "common/FmBank.h"

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {
constexpr uint8_t kCarriers[8] = {0x8, 0x8, 0x8, 0x8, 0xA, 0xE, 0xE, 0xF};
constexpr int kSlotOff[4] = {0, 8, 4, 12};
}

FmChip::FmChip() {
    useFactoryBank();
    reset();
}

void FmChip::useFactoryBank() {
    bank_ = fmbank::chipFactory();
    slot_ = 0;
}

void FmChip::reset() {
    OPN2_Reset(&chip_);
    OPN2_SetChipType(0);
    chanNote_.fill(-1);
    next_ = 0;
    qHead_ = qTail_ = 0;
    push(0, 0x27, 0x00);
    push(0, 0x2B, 0x00);
    writeLfo();
}

const FmChip::Patch& FmChip::patch() const {
    static const Patch fallback;
    if (hasEdit_) return edited_;
    if (bank_.empty()) return fallback;
    return bank_[(size_t) std::clamp(slot_, 0, (int) bank_.size() - 1)].patch;
}

std::string FmChip::patchName(int slot) const {
    if (slot < 0 || slot >= (int) bank_.size()) return {};
    return bank_[(size_t) slot].name;
}

void FmChip::selectPatch(int slot) {
    slot = std::clamp(slot, 0, std::max(0, (int) bank_.size() - 1));
    if (slot == slot_) return;
    slot_ = slot;
    hasEdit_ = false;
    allOff();
}

namespace {
int toChip(int v, int span) { return std::clamp(v * span / 99, 0, span); }
int fromChip(int v, int span) { return std::clamp(v * 99 / std::max(1, span), 0, 99); }
}

FmVoice FmChip::voiceAt(int slot) const {
    static const Patch fallback;
    FmVoice v;
    const Patch& pt = hasEdit_ && slot == slot_ ? edited_
                      : bank_.empty()
                          ? fallback
                          : bank_[(size_t) std::clamp(slot, 0, (int) bank_.size() - 1)].patch;
    v.algorithm = (pt.alg & 7) + 1;
    v.feedback = pt.fb & 7;
    for (int i = 0; i < 4; ++i) {
        const Op& o = pt.ops[(size_t) i];
        FmVoiceOp& op = v.ops[(size_t) i];
        op.level = 99 - std::clamp((int) o.tl * 99 / kSevenBitMax, 0, 99);
        op.ratio = o.mult;
        op.detune = std::clamp((int) o.dt * 2, 0, 14);
        op.attack = fromChip(o.ar, 31);
        op.decay = fromChip(o.dr, 31);
        op.sustain = 99 - fromChip(o.sl, 15);
        op.release = fromChip(o.rr, 15);
    }
    for (int i = 4; i < 6; ++i) v.ops[(size_t) i].level = 0;
    return v;
}

void FmChip::setVoice(const FmVoice& v) {
    Patch p = patch();
    p.alg = (uint8_t) (std::clamp(v.algorithm - 1, 0, 31) % 8);
    p.fb = (uint8_t) std::clamp(v.feedback, 0, 7);
    for (int i = 0; i < 4; ++i) {
        const FmVoiceOp& src = v.ops[(size_t) i];
        Op& o = p.ops[(size_t) i];
        o.tl = (uint8_t) std::clamp(kSevenBitMax - src.level * kSevenBitMax / 99, 0, kSevenBitMax);
        o.mult = (uint8_t) std::clamp(src.ratio, 0, 15);
        o.dt = (uint8_t) std::clamp(src.detune / 2, 0, 6);
        o.ar = (uint8_t) toChip(src.attack, 31);
        o.dr = (uint8_t) toChip(src.decay, 31);
        o.sl = (uint8_t) (15 - toChip(src.sustain, 15));
        o.rr = (uint8_t) toChip(src.release, 15);
    }
    edited_ = p;
    hasEdit_ = true;
    allOff();
}

void FmChip::setMods(const FmMods& m) {
    if (m == mods_) return;
    mods_ = m;
    writeLfo();
}

bool FmChip::loadTfi(const std::string& path) {
    juce::File f(juce::String(juce::CharPointer_UTF8(path.c_str())));
    juce::MemoryBlock raw;
    if (!f.existsAsFile() || !f.loadFileAsData(raw) || raw.getSize() < 42) return false;
    const auto* d = static_cast<const uint8_t*>(raw.getData());
    Patch p;
    p.alg = (uint8_t) (d[0] & 7);
    p.fb = (uint8_t) (d[1] & 7);
    for (int i = 0; i < 4; ++i) {
        const uint8_t* o = d + 2 + i * 10;
        Op& op = p.ops[(size_t) i];
        op.mult = (uint8_t) (o[0] & 15);
        op.dt = (uint8_t) std::min<int>(6, o[1]);
        op.tl = (uint8_t) (o[2] & kSevenBitMax);
        op.rs = (uint8_t) (o[3] & 3);
        op.ar = (uint8_t) (o[4] & 31);
        op.dr = (uint8_t) (o[5] & 31);
        op.d2r = (uint8_t) (o[6] & 31);
        op.rr = (uint8_t) (o[7] & 15);
        op.sl = (uint8_t) (o[8] & 15);
        op.ssg = (uint8_t) (o[9] & 15);
    }
    bank_ = {{f.getFileNameWithoutExtension().toStdString(), p}};
    slot_ = 0;
    allOff();
    return true;
}

bool FmChip::loadWopn(const std::string& path) {
    juce::File f(juce::String(juce::CharPointer_UTF8(path.c_str())));
    juce::MemoryBlock raw;
    if (!f.existsAsFile() || !f.loadFileAsData(raw) || raw.getSize() < 20) return false;
    const auto* d = static_cast<const uint8_t*>(raw.getData());

    fmbank::WopnLayout layout;
    if (!fmbank::wopnLayout(d, raw.getSize(), layout)) return false;

    std::vector<Instrument> loaded;
    for (int i = 0; i < layout.instruments() && (int) loaded.size() < kMaxBank; ++i) {
        const uint8_t* e = d + layout.first + (size_t) i * layout.stride;
        auto name = fmbank::wopnName(e);
        if (name.empty()) continue;
        loaded.push_back({std::move(name), fmbank::wopnPatch(e)});
    }
    if (loaded.empty()) return false;
    bank_ = std::move(loaded);
    slot_ = 0;
    allOff();
    return true;
}

void FmChip::push(uint8_t port, uint8_t reg, uint8_t data) {
    auto put = [this](uint8_t bus, uint8_t value) {
        const int nextTail = (qTail_ + 1) % (int) queue_.size();
        if (nextTail == qHead_) return;
        queue_[(size_t) qTail_] = {bus, value};
        qTail_ = nextTail;
    };
    put((uint8_t) (port * 2), reg);
    put((uint8_t) (port * 2 + 1), data);
}

void FmChip::writeLfo() {
    const int rate = std::clamp((int) std::lround(mods_.speed * 7.0), 0, 7);
    push(0, 0x22, (uint8_t) (mods_.vibrato > 0.0 ? (0x08 | rate) : 0x00));
}

void FmChip::writeChannelPatch(int ch, int velocity, const Patch& pt) {
    const uint8_t port = ch < 3 ? 0 : 1;
    const uint8_t c = (uint8_t) (ch % 3);
    const int lift = (int) std::lround((mods_.bright - 0.5) * 48.0);
    const int slowAtk = (int) std::lround((mods_.attack - 0.5) * 24.0);
    const int slowRel = (int) std::lround((mods_.release - 0.5) * 12.0);
    const int spread = (int) std::lround(mods_.detune / 50.0 * 3.0);
    for (int i = 0; i < 4; ++i) {
        const Op& op = pt.ops[(size_t) i];
        const uint8_t s = (uint8_t) (kSlotOff[i] + c);
        int dtv = (int) op.dt + (spread != 0 ? (i % 2 == 0 ? spread : -spread) : 0);
        dtv = std::clamp(dtv, 0, 6);
        const uint8_t dt = dtv >= 3 ? (uint8_t) (dtv - 3) : (uint8_t) (4 + (3 - dtv));
        push(port, (uint8_t) (0x30 + s), (uint8_t) ((dt << 4) | op.mult));
        const bool carrier = (kCarriers[pt.alg & 7] & (1 << i)) != 0;
        int tl = op.tl;
        if (carrier) tl += (kMidiMax - std::clamp(velocity, 1, kMidiMax)) >> 2;
        else tl -= lift;
        push(port, (uint8_t) (0x40 + s), (uint8_t) std::clamp(tl, 0, kSevenBitMax));
        push(port, (uint8_t) (0x50 + s),
             (uint8_t) ((op.rs << 6) | std::clamp((int) op.ar - slowAtk, 0, 31)));
        push(port, (uint8_t) (0x60 + s), op.dr);
        push(port, (uint8_t) (0x70 + s), op.d2r);
        push(port, (uint8_t) (0x80 + s),
             (uint8_t) ((op.sl << 4) | std::clamp((int) op.rr - slowRel, 0, 15)));
        push(port, (uint8_t) (0x90 + s), op.ssg);
    }
    push(port, (uint8_t) (0xB0 + c), (uint8_t) ((pt.fb << 3) | pt.alg));
    const int pms = std::clamp((int) std::lround(mods_.vibrato * 7.0), 0, 7);
    push(port, (uint8_t) (0xB4 + c), (uint8_t) (0xC0 | pms));
}

FmChip::FreqReg FmChip::freqRegisters(double hz) {
    int block = 1;
    double fnum = hz * 144.0 * (double) (1 << 20) / kMasterClock;
    while (fnum > 2047.0 && block < 8) {
        fnum *= 0.5;
        ++block;
    }
    return {block, std::clamp((int) std::lround(fnum), 0, 2047)};
}

void FmChip::writeFreq(int ch, double hz) {
    const auto r = freqRegisters(hz);
    const uint8_t port = ch < 3 ? 0 : 1;
    const uint8_t c = (uint8_t) (ch % 3);
    push(port, (uint8_t) (0xA4 + c), (uint8_t) (((r.block & 7) << 3) | (r.fnum >> 8)));
    push(port, (uint8_t) (0xA0 + c), (uint8_t) (r.fnum & 255));
}

void FmChip::keyOn(int ch, bool on) {
    const uint8_t code = (uint8_t) ((ch % 3) | (ch < 3 ? 0 : 4));
    push(0, 0x28, (uint8_t) ((on ? 0xF0 : 0x00) | code));
}

bool FmChip::voiceBusy(int ch) const {
    return ch >= 0 && ch < kChannels && chanNote_[(size_t) ch] >= 0;
}

int FmChip::voiceNote(int ch) const {
    return ch >= 0 && ch < kChannels ? chanNote_[(size_t) ch] : -1;
}

void FmChip::voiceOn(int ch, int midinote, int velocity, double hz, const Patch& pt) {
    if (ch < 0 || ch >= kChannels) return;
    if (chanNote_[(size_t) ch] >= 0) keyOn(ch, false);
    chanNote_[(size_t) ch] = midinote;
    chanHz_[(size_t) ch] = hz;
    writeChannelPatch(ch, velocity, pt);
    writeFreq(ch, bentHz(ch));
    keyOn(ch, true);
}

void FmChip::voiceOff(int ch) {
    if (ch < 0 || ch >= kChannels) return;
    keyOn(ch, false);
    chanNote_[(size_t) ch] = -1;
}

void FmChip::retune(int ch, double semitones) {
    if (ch < 0 || ch >= kChannels) return;
    chanBendSemis_[(size_t) ch] = semitones;
    if (voiceBusy(ch)) writeFreq(ch, bentHz(ch));
}

void FmChip::bend(double semitones) {
    for (int ch = 0; ch < kChannels; ++ch) retune(ch, semitones);
}

void FmChip::noteOn(int midinote, int velocity, double hz) {
    int ch = -1;
    for (int i = 0; i < kChannels; ++i)
        if (chanNote_[(size_t) i] < 0) { ch = i; break; }
    if (ch < 0) { ch = next_; next_ = (next_ + 1) % kChannels; }
    voiceOn(ch, midinote, velocity, hz, patch());
}

void FmChip::noteOff(int midinote) {
    for (int i = 0; i < kChannels; ++i)
        if (chanNote_[(size_t) i] == midinote) {
            keyOn(i, false);
            chanNote_[(size_t) i] = -1;
        }
}

void FmChip::allOff() {
    for (int i = 0; i < kChannels; ++i) {
        keyOn(i, false);
        chanNote_[(size_t) i] = -1;
    }
}

void FmChip::render(float* outL, float* outR, int n) {
    constexpr float kScale = 8.0f / 8192.0f;
    for (int i = 0; i < n; ++i) {
        if (qHead_ != qTail_) {
            const Write& w = queue_[(size_t) qHead_];
            qHead_ = (qHead_ + 1) % (int) queue_.size();
            OPN2_Write(&chip_, (Bit32u) w.bus, w.value);
        }
        int sumL = 0, sumR = 0;
        Bit16s frame[2] = {0, 0};
        for (int c = 0; c < 24; ++c) {
            OPN2_Clock(&chip_, frame);
            sumL += frame[0];
            sumR += frame[1];
        }
        outL[i] = (float) sumL * kScale;
        outR[i] = (float) sumR * kScale;
    }
}

}
