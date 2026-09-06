#include "Ph/PhChip.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include <juce_core/juce_core.h>

#include "Ph/PhChipBank.h"

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {
constexpr uint8_t kCarriers[8] = {0x8, 0x8, 0x8, 0x8, 0xA, 0xE, 0xE, 0xF};
constexpr int kSlotOff[4] = {0, 8, 4, 12};
}

PhChip::PhChip() {
    useFactoryBank();
    reset();
}

void PhChip::useFactoryBank() {
    bank_ = phbank::chipFactory();
    slot_ = 0;
}

void PhChip::reset() {
    OPN2_Reset(&chip_);
    OPN2_SetChipType(0);
    chanNote_.fill(-1);
    next_ = 0;
    qHead_ = qTail_ = 0;
    push(0, 0x27, 0x00);
    push(0, 0x2B, 0x00);
    writeLfo();
}

const PhChip::Patch& PhChip::patch() const {
    static const Patch fallback;
    if (hasEdit_) return edited_;
    if (bank_.empty()) return fallback;
    return bank_[(size_t) std::clamp(slot_, 0, (int) bank_.size() - 1)].patch;
}

std::string PhChip::patchName(int slot) const {
    if (slot < 0 || slot >= (int) bank_.size()) return {};
    return bank_[(size_t) slot].name;
}

void PhChip::selectPatch(int slot) {
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

PhVoice PhChip::voiceAt(int slot) const {
    static const Patch fallback;
    PhVoice v;
    const Patch& pt = hasEdit_ && slot == slot_ ? edited_
                      : bank_.empty()
                          ? fallback
                          : bank_[(size_t) std::clamp(slot, 0, (int) bank_.size() - 1)].patch;
    v.algorithm = (pt.alg & 7) + 1;
    v.feedback = pt.fb & 7;
    for (int i = 0; i < 4; ++i) {
        const Op& o = pt.ops[(size_t) i];
        PhVoiceOp& op = v.ops[(size_t) i];
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

void PhChip::setVoice(const PhVoice& v) {
    Patch p = patch();
    p.alg = (uint8_t) (std::clamp(v.algorithm - 1, 0, 31) % 8);
    p.fb = (uint8_t) std::clamp(v.feedback, 0, 7);
    for (int i = 0; i < 4; ++i) {
        const PhVoiceOp& src = v.ops[(size_t) i];
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

void PhChip::setMods(const PhMods& m) {
    if (m == mods_) return;
    mods_ = m;
    writeLfo();
}

bool PhChip::loadTfi(const std::string& path) {
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

namespace {

std::string wopnName(const uint8_t* e) {
    std::string s;
    for (int i = 0; i < 32 && e[i] != 0; ++i)
        s += (e[i] < 32 || e[i] > 126) ? '?' : (char) e[i];
    while (!s.empty() && s.back() == ' ') s.pop_back();
    size_t b = 0;
    while (b < s.size() && s[b] == ' ') ++b;
    return s.substr(b);
}

}

bool PhChip::loadWopn(const std::string& path) {
    juce::File f(juce::String(juce::CharPointer_UTF8(path.c_str())));
    juce::MemoryBlock raw;
    if (!f.existsAsFile() || !f.loadFileAsData(raw) || raw.getSize() < 20) return false;
    const auto* d = static_cast<const uint8_t*>(raw.getData());
    const size_t n = raw.getSize();

    const bool magic2 = std::memcmp(d, "WOPN2-B2NK\0", 11) == 0;
    if (!magic2 && std::memcmp(d, "WOPN2-BANK\0", 11) != 0) return false;
    size_t off = 11;
    int version = 1;
    if (magic2) { version = d[off] | (d[off + 1] << 8); off += 2; }
    const int melodic = (d[off] << 8) | d[off + 1];
    const int percussion = (d[off + 2] << 8) | d[off + 3];
    off += 4 + 1;
    const int banks = melodic + percussion;
    if (banks <= 0 || banks > 128) return false;
    if (version >= 2) off += (size_t) banks * 34;
    const size_t stride = version >= 2 ? 69 : 65;
    if (off + (size_t) banks * 128 * stride > n) return false;

    std::vector<Instrument> loaded;
    for (int i = 0; i < banks * 128 && (int) loaded.size() < kMaxBank; ++i) {
        const uint8_t* e = d + off + (size_t) i * stride;
        auto name = wopnName(e);
        if (name.empty()) continue;
        Patch p;
        p.alg = (uint8_t) (e[35] & 7);
        p.fb = (uint8_t) ((e[35] >> 3) & 7);
        for (int o = 0; o < 4; ++o) {
            const uint8_t* ob = e + 37 + o * 7;
            Op& op = p.ops[(size_t) o];
            const int regDt = (ob[0] >> 4) & 7;
            op.dt = (uint8_t) (regDt <= 3 ? 3 + regDt : 3 - (regDt - 4));
            op.mult = (uint8_t) (ob[0] & 15);
            op.tl = (uint8_t) (ob[1] & kSevenBitMax);
            op.rs = (uint8_t) ((ob[2] >> 6) & 3);
            op.ar = (uint8_t) (ob[2] & 31);
            op.dr = (uint8_t) (ob[3] & 31);
            op.d2r = (uint8_t) (ob[4] & 31);
            op.sl = (uint8_t) ((ob[5] >> 4) & 15);
            op.rr = (uint8_t) (ob[5] & 15);
            op.ssg = ob[6];
        }
        loaded.push_back({std::move(name), p});
    }
    if (loaded.empty()) return false;
    bank_ = std::move(loaded);
    slot_ = 0;
    allOff();
    return true;
}

void PhChip::push(uint8_t port, uint8_t reg, uint8_t data) {
    auto put = [this](uint8_t bus, uint8_t value) {
        const int nextTail = (qTail_ + 1) % (int) queue_.size();
        if (nextTail == qHead_) return;
        queue_[(size_t) qTail_] = {bus, value};
        qTail_ = nextTail;
    };
    put((uint8_t) (port * 2), reg);
    put((uint8_t) (port * 2 + 1), data);
}

void PhChip::writeLfo() {
    const int rate = std::clamp((int) std::lround(mods_.speed * 7.0), 0, 7);
    push(0, 0x22, (uint8_t) (mods_.vibrato > 0.0 ? (0x08 | rate) : 0x00));
}

void PhChip::writeChannelPatch(int ch, int velocity) {
    const uint8_t port = ch < 3 ? 0 : 1;
    const uint8_t c = (uint8_t) (ch % 3);
    const Patch& pt = patch();
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

PhChip::FreqReg PhChip::freqRegisters(double hz) {
    int block = 1;
    double fnum = hz * 144.0 * (double) (1 << 20) / kMasterClock;
    while (fnum > 2047.0 && block < 8) {
        fnum *= 0.5;
        ++block;
    }
    return {block, std::clamp((int) std::lround(fnum), 0, 2047)};
}

void PhChip::writeFreq(int ch, double hz) {
    const auto r = freqRegisters(hz);
    const uint8_t port = ch < 3 ? 0 : 1;
    const uint8_t c = (uint8_t) (ch % 3);
    push(port, (uint8_t) (0xA4 + c), (uint8_t) (((r.block & 7) << 3) | (r.fnum >> 8)));
    push(port, (uint8_t) (0xA0 + c), (uint8_t) (r.fnum & 255));
}

void PhChip::keyOn(int ch, bool on) {
    const uint8_t code = (uint8_t) ((ch % 3) | (ch < 3 ? 0 : 4));
    push(0, 0x28, (uint8_t) ((on ? 0xF0 : 0x00) | code));
}

void PhChip::noteOn(int midinote, int velocity, double hz) {
    int ch = -1;
    for (int i = 0; i < kChannels; ++i)
        if (chanNote_[(size_t) i] < 0) { ch = i; break; }
    if (ch < 0) { ch = next_; next_ = (next_ + 1) % kChannels; keyOn(ch, false); }
    chanNote_[(size_t) ch] = midinote;
    writeChannelPatch(ch, velocity);
    writeFreq(ch, hz);
    keyOn(ch, true);
}

void PhChip::noteOff(int midinote) {
    for (int i = 0; i < kChannels; ++i)
        if (chanNote_[(size_t) i] == midinote) {
            keyOn(i, false);
            chanNote_[(size_t) i] = -1;
        }
}

void PhChip::allOff() {
    for (int i = 0; i < kChannels; ++i) {
        keyOn(i, false);
        chanNote_[(size_t) i] = -1;
    }
}

void PhChip::render(float* outL, float* outR, int n) {
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
