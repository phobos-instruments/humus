#include "Ph/PhOpl.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include <juce_core/juce_core.h>

#include "Ph/PhOplBank.h"

namespace hum {

namespace {
constexpr int kSlotOff[9] = {0, 1, 2, 8, 9, 10, 16, 17, 18};
constexpr int kPairA[6] = {0, 1, 2, 9, 10, 11};

uint16_t regAddr(int ch, uint16_t base, int slotAdd) {
    const uint16_t bank = ch >= 9 ? 0x100 : 0x000;
    return (uint16_t) (bank | (base + kSlotOff[ch % 9] + slotAdd));
}
uint16_t chanAddr(int ch, uint16_t base) {
    const uint16_t bank = ch >= 9 ? 0x100 : 0x000;
    return (uint16_t) (bank | (base + ch % 9));
}
}

PhOpl::PhOpl() {
    useFactoryBank();
    reset();
}

void PhOpl::useFactoryBank() {
    bank_ = phbank::oplFactory();
    slot_ = 0;
}

void PhOpl::reset() {
    OPL3_Reset(&chip_, (uint32_t) std::lround(kRate));
    chanNote_.fill(-1);
    chanHz_.fill(0.0);
    next_ = 0;
    nextPair_ = 0;
    write(0x105, 0x01);
    write(0x104, 0x00);
    write(0x0BD, 0x00);
}

const PhOpl::Patch& PhOpl::patch() const {
    static const Patch fallback;
    if (hasEdit_) return edited_;
    if (bank_.empty()) return fallback;
    return bank_[(size_t) std::clamp(slot_, 0, (int) bank_.size() - 1)].patch;
}

std::string PhOpl::patchName(int slot) const {
    if (slot < 0 || slot >= (int) bank_.size()) return {};
    return bank_[(size_t) slot].name;
}

void PhOpl::selectPatch(int slot) {
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

PhVoice PhOpl::voiceAt(int slot) const {
    static const Patch fallback;
    PhVoice v;
    const Patch& pt = hasEdit_ && slot == slot_ ? edited_
                      : bank_.empty()
                          ? fallback
                          : bank_[(size_t) std::clamp(slot, 0, (int) bank_.size() - 1)].patch;
    v.algorithm = (pt.fbcon1 & 1) + 1;
    v.feedback = (pt.fbcon1 >> 1) & 7;
    const int nOps = pt.fourOp ? 4 : 2;
    for (int i = 0; i < nOps; ++i) {
        const Op& o = pt.ops[(size_t) i];
        PhVoiceOp& op = v.ops[(size_t) i];
        op.level = 99 - std::clamp((int) (o.ksltl & 63) * 99 / 63, 0, 99);
        op.ratio = o.avekm & 15;
        op.detune = 7;
        op.attack = fromChip((o.ardr >> 4) & 15, 15);
        op.decay = fromChip(o.ardr & 15, 15);
        op.sustain = 99 - fromChip((o.slrr >> 4) & 15, 15);
        op.release = fromChip(o.slrr & 15, 15);
    }
    for (int i = nOps; i < 6; ++i) v.ops[(size_t) i].level = 0;
    return v;
}

void PhOpl::setVoice(const PhVoice& v) {
    Patch p = patch();
    p.fbcon1 = (uint8_t) ((std::clamp(v.feedback, 0, 7) << 1)
                          | (std::clamp(v.algorithm - 1, 0, 31) & 1));
    const int nOps = p.fourOp ? 4 : 2;
    for (int i = 0; i < nOps; ++i) {
        const PhVoiceOp& src = v.ops[(size_t) i];
        Op& o = p.ops[(size_t) i];
        o.ksltl = (uint8_t) ((o.ksltl & 0xC0)
                             | std::clamp(63 - src.level * 63 / 99, 0, 63));
        o.avekm = (uint8_t) ((o.avekm & 0xF0) | std::clamp(src.ratio, 0, 15));
        o.ardr = (uint8_t) ((toChip(src.attack, 15) << 4) | toChip(src.decay, 15));
        o.slrr = (uint8_t) (((15 - toChip(src.sustain, 15)) << 4)
                            | toChip(src.release, 15));
    }
    edited_ = p;
    hasEdit_ = true;
    allOff();
}

void PhOpl::setMods(const PhMods& m) {
    if (m == mods_) return;
    mods_ = m;
    write(0x0BD, (uint8_t) (m.vibrato > 0.5 ? 0x40 : 0x00));
}

bool PhOpl::parseWopl(const uint8_t* d, size_t n) {
    if (n < 19 || std::memcmp(d, "WOPL3-BANK\0", 11) != 0) return false;
    const int version = d[11] | (d[12] << 8);
    const int melodic = (d[13] << 8) | d[14];
    const int percussion = (d[15] << 8) | d[16];
    size_t off = 19;
    const int banks = melodic + percussion;
    if (melodic <= 0 || banks > 128) return false;
    if (version >= 2) off += (size_t) banks * 34;
    const size_t stride = version >= 3 ? 66 : 62;
    if (off + (size_t) melodic * 128 * stride > n) return false;

    std::vector<Instrument> loaded;
    for (int i = 0; i < melodic * 128 && (int) loaded.size() < kMaxBank; ++i) {
        const uint8_t* e = d + off + (size_t) i * stride;
        std::string name;
        for (int c = 0; c < 32 && e[c] != 0; ++c)
            name += (e[c] < 32 || e[c] > 126) ? '?' : (char) e[c];
        while (!name.empty() && name.back() == ' ') name.pop_back();
        const uint8_t flags = e[39];
        if (name.empty() || (flags & 4) != 0) continue;
        Patch p;
        p.fourOp = (flags & 1) != 0 && (flags & 2) == 0;
        p.noteOffset = (int8_t) e[33];
        p.fbcon1 = e[40];
        p.fbcon2 = e[41];
        for (int o = 0; o < 4; ++o) {
            const uint8_t* ob = e + 42 + o * 5;
            Op& op = p.ops[(size_t) o];
            op.avekm = ob[0];
            op.ksltl = ob[1];
            op.ardr = ob[2];
            op.slrr = ob[3];
            op.ws = (uint8_t) (ob[4] & 7);
        }
        loaded.push_back({std::move(name), p});
    }
    if (loaded.empty()) return false;
    bank_ = std::move(loaded);
    slot_ = 0;
    allOff();
    return true;
}

bool PhOpl::loadWopl(const std::string& path) {
    juce::File f(juce::String(juce::CharPointer_UTF8(path.c_str())));
    juce::MemoryBlock raw;
    if (!f.existsAsFile() || !f.loadFileAsData(raw)) return false;
    return parseWopl(static_cast<const uint8_t*>(raw.getData()), raw.getSize());
}

void PhOpl::write(uint16_t reg, uint8_t data) {
    OPL3_WriteRegBuffered(&chip_, reg, data);
}

void PhOpl::writeOp(int ch, int opIndex, const Op& op, bool carrier, int velocity) {
    const int slotAdd = opIndex == 0 ? 0 : 3;
    const int lift = (int) std::lround((mods_.bright - 0.5) * 24.0);
    const int slowAtk = (int) std::lround((mods_.attack - 0.5) * 10.0);
    const int slowRel = (int) std::lround((mods_.release - 0.5) * 6.0);
    write(regAddr(ch, 0x20, slotAdd), op.avekm);
    int tl = op.ksltl & 63;
    if (carrier) tl += (127 - std::clamp(velocity, 1, 127)) >> 3;
    else tl -= lift;
    write(regAddr(ch, 0x40, slotAdd),
          (uint8_t) ((op.ksltl & 0xC0) | std::clamp(tl, 0, 63)));
    const int ar = std::clamp((int) ((op.ardr >> 4) & 15) - slowAtk, 1, 15);
    write(regAddr(ch, 0x60, slotAdd), (uint8_t) ((ar << 4) | (op.ardr & 15)));
    const int rr = std::clamp((int) (op.slrr & 15) - slowRel, 1, 15);
    write(regAddr(ch, 0x80, slotAdd), (uint8_t) ((op.slrr & 0xF0) | rr));
    write(regAddr(ch, 0xE0, slotAdd), op.ws);
}

void PhOpl::writeChannelPatch(int ch, bool secondary, int velocity) {
    const Patch& pt = patch();
    const int base = secondary ? 2 : 0;
    const uint8_t fbcon = secondary ? pt.fbcon2 : pt.fbcon1;
    const bool amConn = (fbcon & 1) != 0;
    writeOp(ch, 0, pt.ops[(size_t) base], amConn, velocity);
    writeOp(ch, 1, pt.ops[(size_t) (base + 1)], true, velocity);
    write(chanAddr(ch, 0xC0), (uint8_t) (0x30 | (fbcon & 0x0F)));
}

PhOpl::FreqReg PhOpl::freqRegisters(double hz) {
    int block = 1;
    double fnum = hz * (double) (1 << 19) / kRate;
    while (fnum > 1023.0 && block < 7) {
        fnum *= 0.5;
        ++block;
    }
    return {block, std::clamp((int) std::lround(fnum), 0, 1023)};
}

void PhOpl::writeFreq(int ch, double hz, bool on) {
    const auto r = freqRegisters(hz);
    write(chanAddr(ch, 0xA0), (uint8_t) (r.fnum & 255));
    write(chanAddr(ch, 0xB0),
          (uint8_t) ((on ? 0x20 : 0x00) | ((r.block & 7) << 2) | (r.fnum >> 8)));
}

int PhOpl::pairOf(int ch) {
    const int c9 = ch % 9;
    if (c9 >= 6) return -1;
    return c9 % 3 + (ch >= 9 ? 3 : 0);
}

void PhOpl::noteOn(int midinote, int velocity, double hz) {
    const Patch& pt = patch();
    hz *= std::pow(2.0, (double) pt.noteOffset / 12.0);
    if (pt.fourOp) {
        int pair = -1;
        for (int i = 0; i < kPairs; ++i)
            if (chanNote_[(size_t) kPairA[i]] < 0
                && chanNote_[(size_t) (kPairA[i] + 3)] < 0) { pair = i; break; }
        if (pair < 0) {
            pair = nextPair_;
            nextPair_ = (nextPair_ + 1) % kPairs;
            writeFreq(kPairA[pair], chanHz_[(size_t) kPairA[pair]], false);
        }
        const int a = kPairA[pair];
        const int b = a + 3;
        fourOpMask_ |= (uint8_t) (1 << pair);
        write(0x104, fourOpMask_);
        chanNote_[(size_t) a] = midinote;
        chanNote_[(size_t) b] = -2;
        chanHz_[(size_t) a] = hz;
        writeChannelPatch(a, false, velocity);
        writeChannelPatch(b, true, velocity);
        writeFreq(b, hz, false);
        writeFreq(a, hz, true);
        return;
    }
    static constexpr int kPrefer[kChannels] = {6, 7, 8, 15, 16, 17, 3, 4, 5,
                                               12, 13, 14, 0, 1, 2, 9, 10, 11};
    int ch = -1;
    for (int i = 0; i < kChannels; ++i) {
        const int c = kPrefer[i];
        const int pair = pairOf(c);
        if (chanNote_[(size_t) c] < 0
            && (pair < 0 || (fourOpMask_ & (1 << pair)) == 0)) { ch = c; break; }
    }
    if (ch < 0) {
        ch = kPrefer[next_ % 6];
        next_ = (next_ + 1) % 6;
        writeFreq(ch, chanHz_[(size_t) ch], false);
    }
    chanNote_[(size_t) ch] = midinote;
    chanHz_[(size_t) ch] = hz;
    writeChannelPatch(ch, false, velocity);
    writeFreq(ch, hz, true);
}

void PhOpl::noteOff(int midinote) {
    for (int i = 0; i < kChannels; ++i)
        if (chanNote_[(size_t) i] == midinote) {
            writeFreq(i, chanHz_[(size_t) i], false);
            chanNote_[(size_t) i] = -1;
            const int pair = pairOf(i);
            if (pair >= 0 && (fourOpMask_ & (1 << pair)) != 0) {
                if (chanNote_[(size_t) (i + 3)] == -2) chanNote_[(size_t) (i + 3)] = -1;
                fourOpMask_ &= (uint8_t) ~(1 << pair);
                write(0x104, fourOpMask_);
            }
        }
}

void PhOpl::allOff() {
    for (int i = 0; i < kChannels; ++i) {
        writeFreq(i, chanHz_[(size_t) i] > 0.0 ? chanHz_[(size_t) i] : 440.0, false);
        chanNote_[(size_t) i] = -1;
    }
    fourOpMask_ = 0;
    write(0x104, 0x00);
}

void PhOpl::render(float* outL, float* outR, int n) {
    constexpr float kScale = 1.0f / 16384.0f;
    for (int i = 0; i < n; ++i) {
        int16_t frame[2] = {0, 0};
        OPL3_Generate(&chip_, frame);
        outL[i] = (float) frame[0] * kScale;
        outR[i] = (float) frame[1] * kScale;
    }
}

}
