#include "Ph/PhOpm.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>

#include <juce_core/juce_core.h>

#include "Ph/PhOpmBank.h"

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {
constexpr uint8_t kCarriers[8] = {0x8, 0x8, 0x8, 0x8, 0xA, 0xE, 0xE, 0xF};
constexpr int kRegOp[4] = {0, 2, 1, 3};
constexpr int kKeyIndexA4 = 4 * 12 + 8;
}

PhOpm::PhOpm() {
    useFactoryBank();
    reset();
}

void PhOpm::useFactoryBank() {
    bank_ = phbank::opmFactory();
    slot_ = 0;
}

void PhOpm::reset() {
    OPM_Reset(&chip_, opp_ ? opm_flags_ym2164 : opm_flags_none);
    chanNote_.fill(-1);
    next_ = 0;
    qHead_ = qTail_ = 0;
    writeLfo();
}

void PhOpm::setVariant(bool opp) {
    if (opp == opp_) return;
    opp_ = opp;
    reset();
}

const PhOpm::Patch& PhOpm::patch() const {
    static const Patch fallback;
    if (hasEdit_) return edited_;
    if (bank_.empty()) return fallback;
    return bank_[(size_t) std::clamp(slot_, 0, (int) bank_.size() - 1)].patch;
}

std::string PhOpm::patchName(int slot) const {
    if (slot < 0 || slot >= (int) bank_.size()) return {};
    return bank_[(size_t) slot].name;
}

void PhOpm::selectPatch(int slot) {
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

PhVoice PhOpm::voiceAt(int slot) const {
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
        op.detune = std::clamp((int) o.dt1 * 2, 0, 14);
        op.attack = fromChip(o.ar, 31);
        op.decay = fromChip(o.d1r, 31);
        op.sustain = 99 - fromChip(o.d1l, 15);
        op.release = fromChip(o.rr, 15);
    }
    for (int i = 4; i < 6; ++i) v.ops[(size_t) i].level = 0;
    return v;
}

void PhOpm::setVoice(const PhVoice& v) {
    Patch p = patch();
    p.alg = (uint8_t) (std::clamp(v.algorithm - 1, 0, 31) % 8);
    p.fb = (uint8_t) std::clamp(v.feedback, 0, 7);
    for (int i = 0; i < 4; ++i) {
        const PhVoiceOp& src = v.ops[(size_t) i];
        Op& o = p.ops[(size_t) i];
        o.tl = (uint8_t) std::clamp(kSevenBitMax - src.level * kSevenBitMax / 99, 0, kSevenBitMax);
        o.mult = (uint8_t) std::clamp(src.ratio, 0, 15);
        o.dt1 = (uint8_t) std::clamp(src.detune / 2, 0, 6);
        o.ar = (uint8_t) toChip(src.attack, 31);
        o.d1r = (uint8_t) toChip(src.decay, 31);
        o.d1l = (uint8_t) (15 - toChip(src.sustain, 15));
        o.rr = (uint8_t) toChip(src.release, 15);
    }
    edited_ = p;
    hasEdit_ = true;
    allOff();
}

void PhOpm::setMods(const PhMods& m) {
    if (m == mods_) return;
    mods_ = m;
    writeLfo();
}

bool PhOpm::parseOpm(const std::string& text, const std::string& fallbackName) {
    std::vector<Instrument> loaded;
    Instrument cur;
    bool open = false;
    auto commit = [&] {
        if (open && (int) loaded.size() < kMaxBank) loaded.push_back(cur);
        open = false;
    };
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
        if (line.rfind("//", 0) == 0 || line.empty()) continue;
        const auto colon = line.find(':');
        if (colon == std::string::npos) continue;
        const std::string tag = line.substr(0, colon);
        std::istringstream row(line.substr(colon + 1));
        if (tag == "@") {
            commit();
            cur = Instrument{};
            int num = 0;
            row >> num;
            std::string name;
            std::getline(row, name);
            const auto b = name.find_first_not_of(' ');
            cur.name = b == std::string::npos ? fallbackName : name.substr(b);
            if (cur.name.empty()) cur.name = fallbackName;
            open = true;
            continue;
        }
        if (!open) continue;
        std::array<int, 11> f{};
        int got = 0;
        while (got < (int) f.size() && (row >> f[(size_t) got])) ++got;
        if (tag == "LFO" && got >= 4) {
            cur.patch.lfrq = (uint8_t) std::clamp(f[0], 0, 255);
            cur.patch.amd = (uint8_t) std::clamp(f[1], 0, kSevenBitMax);
            cur.patch.pmd = (uint8_t) std::clamp(f[2], 0, kSevenBitMax);
            cur.patch.wf = (uint8_t) (f[3] & 3);
        } else if (tag == "CH" && got >= 5) {
            cur.patch.fb = (uint8_t) (f[1] & 7);
            cur.patch.alg = (uint8_t) (f[2] & 7);
            cur.patch.ams = (uint8_t) (f[3] & 3);
            cur.patch.pms = (uint8_t) (f[4] & 7);
        } else if ((tag == "M1" || tag == "C1" || tag == "M2" || tag == "C2") && got >= 10) {
            const int i = tag == "M1" ? 0 : tag == "C1" ? 1 : tag == "M2" ? 2 : 3;
            Op& o = cur.patch.ops[(size_t) i];
            o.ar = (uint8_t) (f[0] & 31);
            o.d1r = (uint8_t) (f[1] & 31);
            o.d2r = (uint8_t) (f[2] & 31);
            o.rr = (uint8_t) (f[3] & 15);
            o.d1l = (uint8_t) (f[4] & 15);
            o.tl = (uint8_t) (f[5] & kSevenBitMax);
            o.ks = (uint8_t) (f[6] & 3);
            o.mult = (uint8_t) (f[7] & 15);
            const int regDt = f[8] & 7;
            o.dt1 = (uint8_t) (regDt <= 3 ? 3 + regDt : 3 - (regDt - 4));
            o.dt2 = (uint8_t) (f[9] & 3);
            if (got >= 11) o.ame = (uint8_t) (f[10] != 0 ? 1 : 0);
        }
    }
    commit();
    if (loaded.empty()) return false;
    bank_ = std::move(loaded);
    slot_ = 0;
    allOff();
    return true;
}

bool PhOpm::loadOpm(const std::string& path) {
    juce::File f(juce::String(juce::CharPointer_UTF8(path.c_str())));
    if (!f.existsAsFile()) return false;
    return parseOpm(f.loadFileAsString().toStdString(),
                    f.getFileNameWithoutExtension().toStdString());
}

void PhOpm::push(uint8_t reg, uint8_t data) {
    auto put = [this](uint8_t bus, uint8_t value) {
        const int nextTail = (qTail_ + 1) % (int) queue_.size();
        if (nextTail == qHead_) return;
        queue_[(size_t) qTail_] = {bus, value};
        qTail_ = nextTail;
    };
    put(0, reg);
    put(1, data);
}

void PhOpm::writeLfo() {
    const Patch& pt = patch();
    const int lfrq = std::clamp((int) pt.lfrq + (int) std::lround((mods_.speed - 0.5) * 128.0),
                                0, 255);
    push(0x18, (uint8_t) lfrq);
    push(0x1B, (uint8_t) (pt.wf & 3));
    const int pmd = std::clamp((int) pt.pmd + (int) std::lround(mods_.vibrato * 64.0), 0, kSevenBitMax);
    push(0x19, (uint8_t) (0x80 | pmd));
    push(0x19, (uint8_t) (pt.amd & kSevenBitMax));
}

void PhOpm::writeChannelPatch(int ch, int velocity) {
    const Patch& pt = patch();
    const int lift = (int) std::lround((mods_.bright - 0.5) * 48.0);
    const int slowAtk = (int) std::lround((mods_.attack - 0.5) * 24.0);
    const int slowRel = (int) std::lround((mods_.release - 0.5) * 12.0);
    const int spread = (int) std::lround(mods_.detune / 50.0 * 3.0);
    for (int i = 0; i < 4; ++i) {
        const Op& op = pt.ops[(size_t) i];
        const uint8_t s = (uint8_t) (kRegOp[i] * 8 + ch);
        int dtv = (int) op.dt1 + (spread != 0 ? (i % 2 == 0 ? spread : -spread) : 0);
        dtv = std::clamp(dtv, 0, 6);
        const uint8_t dt = dtv >= 3 ? (uint8_t) (dtv - 3) : (uint8_t) (4 + (3 - dtv));
        push((uint8_t) (0x40 + s), (uint8_t) ((dt << 4) | op.mult));
        const bool carrier = (kCarriers[pt.alg & 7] & (1 << i)) != 0;
        int tl = op.tl;
        if (carrier) tl += (kMidiMax - std::clamp(velocity, 1, kMidiMax)) >> 2;
        else tl -= lift;
        push((uint8_t) (0x60 + s), (uint8_t) std::clamp(tl, 0, kSevenBitMax));
        push((uint8_t) (0x80 + s),
             (uint8_t) ((op.ks << 6) | std::clamp((int) op.ar - slowAtk, 0, 31)));
        push((uint8_t) (0xA0 + s), (uint8_t) ((op.ame << 7) | op.d1r));
        push((uint8_t) (0xC0 + s), (uint8_t) ((op.dt2 << 6) | op.d2r));
        push((uint8_t) (0xE0 + s),
             (uint8_t) ((op.d1l << 4) | std::clamp((int) op.rr - slowRel, 0, 15)));
    }
    push((uint8_t) (0x20 + ch), (uint8_t) (0xC0 | (pt.fb << 3) | pt.alg));
    const int pms = std::max((int) pt.pms,
                             std::clamp((int) std::lround(mods_.vibrato * 7.0), 0, 7));
    push((uint8_t) (0x38 + ch), (uint8_t) ((pms << 4) | (pt.ams & 3)));
}

PhOpm::KeyReg PhOpm::keyRegisters(double hz) {
    const double semis = 12.0 * std::log2(std::max(1.0, hz) / kA4Hz);
    const double pos = (double) kKeyIndexA4 + semis;
    int whole = (int) std::floor(pos);
    int frac = (int) std::lround((pos - (double) whole) * 64.0);
    if (frac >= 64) { frac -= 64; ++whole; }
    whole = std::clamp(whole, 0, 8 * 12 - 1);
    const int octave = whole / 12;
    const int note = whole % 12;
    return {(octave << 4) | (note + note / 3), frac};
}

void PhOpm::writeKey(int ch, double hz) {
    const auto r = keyRegisters(hz);
    push((uint8_t) (0x28 + ch), (uint8_t) r.kc);
    push((uint8_t) (0x30 + ch), (uint8_t) (r.kf << 2));
}

void PhOpm::keyOn(int ch, bool on) {
    push(0x08, (uint8_t) ((on ? 0x78 : 0x00) | ch));
}

void PhOpm::noteOn(int midinote, int velocity, double hz) {
    int ch = -1;
    for (int i = 0; i < kChannels; ++i)
        if (chanNote_[(size_t) i] < 0) { ch = i; break; }
    if (ch < 0) { ch = next_; next_ = (next_ + 1) % kChannels; keyOn(ch, false); }
    chanNote_[(size_t) ch] = midinote;
    writeChannelPatch(ch, velocity);
    writeKey(ch, hz);
    keyOn(ch, true);
}

void PhOpm::noteOff(int midinote) {
    for (int i = 0; i < kChannels; ++i)
        if (chanNote_[(size_t) i] == midinote) {
            keyOn(i, false);
            chanNote_[(size_t) i] = -1;
        }
}

void PhOpm::allOff() {
    for (int i = 0; i < kChannels; ++i) {
        keyOn(i, false);
        chanNote_[(size_t) i] = -1;
    }
}

void PhOpm::render(float* outL, float* outR, int n) {
    constexpr float kScale = 1.0f / (32.0f * 16384.0f);
    for (int i = 0; i < n; ++i) {
        if (qHead_ != qTail_) {
            const Write& w = queue_[(size_t) qHead_];
            qHead_ = (qHead_ + 1) % (int) queue_.size();
            OPM_Write(&chip_, w.bus, w.value);
        }
        long sumL = 0, sumR = 0;
        int32_t frame[2] = {0, 0};
        uint8_t sh1 = 0, sh2 = 0, so = 0;
        for (int c = 0; c < 32; ++c) {
            OPM_Clock(&chip_, frame, &sh1, &sh2, &so);
            sumL += frame[0];
            sumR += frame[1];
        }
        outL[i] = (float) sumL * kScale;
        outR[i] = (float) sumR * kScale;
    }
}

}
