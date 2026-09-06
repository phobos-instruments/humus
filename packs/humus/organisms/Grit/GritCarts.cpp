#include "Grit/GritImpl.h"

#include <cmath>
#include <cstdint>

namespace hum {

namespace {

constexpr int kVrc6Duty[4] = {1, 3, 7, 7};

int waveSample(int wave, int i, int steps, int peak) {
    const double t = (double) i / (double) steps;
    double v = 0.0;
    switch (wave) {
        case 0: v = std::sin(2.0 * 3.14159265358979 * t); break;
        case 1: v = t < 0.5 ? 4.0 * t - 1.0 : 3.0 - 4.0 * t; break;
        case 2: v = 2.0 * t - 1.0; break;
        case 3: v = t < 0.5 ? 1.0 : -1.0; break;
        default:
            v = 0.6 * std::sin(2.0 * 3.14159265358979 * t)
              + 0.3 * std::sin(4.0 * 3.14159265358979 * t)
              + 0.1 * std::sin(8.0 * 3.14159265358979 * t);
            break;
    }
    return std::clamp((int) std::lround((v * 0.5 + 0.5) * peak), 0, peak);
}

}

int Grit::voiceCount() const {
    switch (cart()) {
        case GritCart::kVrc6: return 5;
        case GritCart::kMmc5: return 4;
        case GritCart::kFds: return 3;
        case GritCart::kN163: return 6;
        case GritCart::k5B: return 5;
        case GritCart::kVrc7: return 8;
        default: return 2;
    }
}

GritTarget Grit::target(int voice) const {
    if (voice < 2) return {GritTarget::kPulse, voice};
    const int x = voice - 2;
    switch (cart()) {
        case GritCart::kVrc6:
            return x < 2 ? GritTarget{GritTarget::kVrc6Pulse, x}
                         : GritTarget{GritTarget::kVrc6Saw, 0};
        case GritCart::kMmc5: return {GritTarget::kMmc5Pulse, x};
        case GritCart::kFds: return {GritTarget::kFds, 0};
        case GritCart::kN163: return {GritTarget::kN163, 4 + x};
        case GritCart::k5B: return {GritTarget::kFme7, x};
        case GritCart::kVrc7: return {GritTarget::kVrc7, x};
        default: return {GritTarget::kPulse, 0};
    }
}

void Grit::pokeExp(std::uint32_t adr, int value) {
    const auto a = (xgm::UINT32) adr;
    const auto d = (xgm::UINT32) value;
    switch (cart()) {
        case GritCart::kVrc6: impl_->vrc6.Write(a, d); break;
        case GritCart::kMmc5: impl_->mmc5.Write(a, d); break;
        case GritCart::kFds: impl_->fds.Write(a, d); break;
        case GritCart::kN163: impl_->n163.Write(a, d); break;
        case GritCart::k5B: impl_->fme7.Write(a, d); break;
        case GritCart::kVrc7: impl_->vrc7.Write(a, d); break;
        default: break;
    }
}

void Grit::applyCartSetup() {
    cachedCart_ = (int) cart();
    for (auto& v : voices_) {
        v.note = -1;
        v.sounding = false;
        v.level = 0;
        v.lastHi = -1;
    }
    switch (cart()) {
        case GritCart::kVrc6:
            impl_->vrc6.Reset();
            break;
        case GritCart::kMmc5:
            impl_->mmc5.Reset();
            impl_->mmc5.Write(0x5015, 0x03);
            break;
        case GritCart::kFds:
            impl_->fds.Reset();
            break;
        case GritCart::kN163:
            impl_->n163.Reset();
            impl_->n163.Write(0xE000, 0x00);
            break;
        case GritCart::k5B:
            impl_->fme7.Reset();
            impl_->fme7.Write(0xC000, 0x07);
            impl_->fme7.Write(0xE000, 0x38);
            break;
        case GritCart::kVrc7:
            impl_->vrc7.Reset();
            break;
        default: break;
    }
    cachedWave_ = -1;
    applyWavetables();
}

void Grit::applyWavetables() {
    const int wave = std::clamp((int) params.get("Wave", 0.0), 0, 4);
    cachedWave_ = wave;
    if (cart() == GritCart::kFds) {
        impl_->fds.Write(0x4083, 0x80);
        impl_->fds.Write(0x4089, 0x80);
        for (int i = 0; i < 64; ++i)
            impl_->fds.Write((xgm::UINT32) (0x4040 + i), (xgm::UINT32) waveSample(wave, i, 64, 63));
        impl_->fds.Write(0x4089, 0x00);
        impl_->fds.Write(0x4084, 0x80);
        impl_->fds.Write(0x4087, 0x80);
        impl_->fds.Write(0x408A, 0xFF);
        impl_->fds.Write(0x4083, 0x00);
    } else if (cart() == GritCart::kN163) {
        impl_->n163.Write(0xF800, 0x80);
        for (int i = 0; i < 16; ++i) {
            const int lo = waveSample(wave, 2 * i, 32, 15);
            const int hi = waveSample(wave, 2 * i + 1, 32, 15);
            impl_->n163.Write(0x4800, (xgm::UINT32) ((hi << 4) | lo));
        }
        for (int slot = 4; slot < 8; ++slot) {
            const int base = 0x40 + 8 * slot;
            impl_->n163.Write(0xF800, (xgm::UINT32) (base + 6));
            impl_->n163.Write(0x4800, 0x00);
        }
        impl_->n163.Write(0xF800, 0x7F);
        impl_->n163.Write(0x4800, 0x30);
    }
}

void Grit::applyPitch(int v) {
    const auto& vc = voices_[(size_t) v];
    if (vc.hz <= 0.0) return;
    const auto t = target(v);
    const double c = clock();
    switch (t.kind) {
        case GritTarget::kPulse: {
            const int p = std::clamp((int) std::lround(c / (16.0 * vc.hz)) - 1, 8, 2047);
            pokeApu(t.ch * 4 + 2, p & 255);
            if ((p >> 8) != vc.lastHi) {
                voices_[(size_t) v].lastHi = p >> 8;
                pokeApu(t.ch * 4 + 3, (p >> 8) | 0x08);
            }
            break;
        }
        case GritTarget::kMmc5Pulse: {
            const int p = std::clamp((int) std::lround(c / (16.0 * vc.hz)) - 1, 8, 2047);
            pokeExp((std::uint32_t) (0x5000 + t.ch * 4 + 2), p & 255);
            if ((p >> 8) != vc.lastHi) {
                voices_[(size_t) v].lastHi = p >> 8;
                pokeExp((std::uint32_t) (0x5000 + t.ch * 4 + 3), (p >> 8) | 0x08);
            }
            break;
        }
        case GritTarget::kVrc6Pulse: {
            const int p = std::clamp((int) std::lround(c / (16.0 * vc.hz)) - 1, 0, 4095);
            const std::uint32_t base = t.ch == 0 ? 0x9000u : 0xA000u;
            pokeExp(base + 1, p & 255);
            pokeExp(base + 2, 0x80 | (p >> 8));
            break;
        }
        case GritTarget::kVrc6Saw: {
            const int p = std::clamp((int) std::lround(c / (14.0 * vc.hz)) - 1, 0, 4095);
            pokeExp(0xB001, p & 255);
            pokeExp(0xB002, 0x80 | (p >> 8));
            break;
        }
        case GritTarget::kFds: {
            const int f = std::clamp((int) std::lround(vc.hz * 65536.0 * 64.0 / c), 0,
                                     4095);
            pokeExp(0x4082, f & 255);
            pokeExp(0x4083, f >> 8);
            break;
        }
        case GritTarget::kN163: {
            const int f = std::clamp(
                (int) std::lround(vc.hz * 15.0 * 65536.0 * 4.0 * 32.0 / c), 0, 262143);
            const int base = 0x40 + 8 * t.ch;
            pokeExp(0xF800, (std::uint32_t) base);
            pokeExp(0x4800, f & 255);
            pokeExp(0xF800, (std::uint32_t) (base + 2));
            pokeExp(0x4800, (f >> 8) & 255);
            pokeExp(0xF800, (std::uint32_t) (base + 4));
            pokeExp(0x4800, 0xE0 | ((f >> 16) & 3));
            break;
        }
        case GritTarget::kFme7: {
            const int p = std::clamp((int) std::lround(c / (32.0 * vc.hz)), 1, 4095);
            pokeExp(0xC000, (std::uint32_t) (t.ch * 2));
            pokeExp(0xE000, p & 255);
            pokeExp(0xC000, (std::uint32_t) (t.ch * 2 + 1));
            pokeExp(0xE000, p >> 8);
            break;
        }
        case GritTarget::kVrc7: {
            int oct = 0;
            double f = vc.hz;
            while (f >= 2.0 * 49716.0 * 512.0 / 524288.0 && oct < 7) {
                f *= 0.5;
                ++oct;
            }
            const int fnum =
                std::clamp((int) std::lround(f * 524288.0 / 49716.0), 0, 511);
            pokeExp(0x9010, (std::uint32_t) (0x10 + t.ch));
            pokeExp(0x9030, fnum & 255);
            voices_[(size_t) v].lastHi = 0x10 | (oct << 1) | (fnum >> 8);
            pokeExp(0x9010, (std::uint32_t) (0x20 + t.ch));
            pokeExp(0x9030, (std::uint32_t) voices_[(size_t) v].lastHi);
            break;
        }
    }
}

void Grit::applyLevel(int v) {
    const auto& vc = voices_[(size_t) v];
    const int lvl = std::clamp((int) std::lround(vc.level * vc.vel), 0, 15);
    const auto t = target(v);
    const int duty = std::clamp((int) params.get("Duty", 2.0), 0, 3);
    switch (t.kind) {
        case GritTarget::kPulse:
            pokeApu(t.ch * 4, (duty << 6) | 0x30 | lvl);
            break;
        case GritTarget::kMmc5Pulse:
            pokeExp((std::uint32_t) (0x5000 + t.ch * 4), (duty << 6) | 0x30 | lvl);
            break;
        case GritTarget::kVrc6Pulse: {
            const std::uint32_t base = t.ch == 0 ? 0x9000u : 0xA000u;
            pokeExp(base, (kVrc6Duty[duty] << 4) | lvl);
            break;
        }
        case GritTarget::kVrc6Saw:
            pokeExp(0xB000, std::clamp(lvl * 42 / 15, 0, 42));
            break;
        case GritTarget::kFds:
            pokeExp(0x4080, 0x80 | std::clamp(lvl * 32 / 15, 0, 32));
            break;
        case GritTarget::kN163: {
            const int base = 0x40 + 8 * t.ch;
            pokeExp(0xF800, (std::uint32_t) (base + 7));
            pokeExp(0x4800,
                    base + 7 == 0x7F ? (std::uint32_t) (0x30 | lvl) : (std::uint32_t) lvl);
            break;
        }
        case GritTarget::kFme7:
            pokeExp(0xC000, (std::uint32_t) (8 + t.ch));
            pokeExp(0xE000, (std::uint32_t) (lvl >= 15 ? 15 : lvl));
            break;
        case GritTarget::kVrc7: {
            const int patch = std::clamp((int) params.get("Patch", 1.0), 1, 15);
            pokeExp(0x9010, (std::uint32_t) (0x30 + t.ch));
            pokeExp(0x9030, (std::uint32_t) ((patch << 4) | (15 - lvl)));
            break;
        }
    }
}

void Grit::triggerVoice(int v) {
    const auto t = target(v);
    if (t.kind == GritTarget::kPulse) {
        voices_[(size_t) v].lastHi = -1;
        applyPitch(v);
    } else if (t.kind == GritTarget::kMmc5Pulse) {
        voices_[(size_t) v].lastHi = -1;
        applyPitch(v);
    } else if (t.kind == GritTarget::kVrc7) {
        pokeExp(0x9010, (std::uint32_t) (0x20 + t.ch));
        pokeExp(0x9030, 0x00);
        applyPitch(v);
    }
}

void Grit::silenceVoice(int v) {
    const auto t = target(v);
    voices_[(size_t) v].sounding = false;
    voices_[(size_t) v].level = 0;
    switch (t.kind) {
        case GritTarget::kPulse: pokeApu(t.ch * 4, 0x30); break;
        case GritTarget::kMmc5Pulse:
            pokeExp((std::uint32_t) (0x5000 + t.ch * 4), 0x30);
            break;
        case GritTarget::kVrc6Pulse:
            pokeExp(t.ch == 0 ? 0x9000u : 0xA000u, 0x00);
            break;
        case GritTarget::kVrc6Saw: pokeExp(0xB000, 0x00); break;
        case GritTarget::kFds: pokeExp(0x4080, 0x80); break;
        case GritTarget::kN163: {
            const int base = 0x40 + 8 * t.ch;
            pokeExp(0xF800, (std::uint32_t) (base + 7));
            pokeExp(0x4800, base + 7 == 0x7F ? 0x30u : 0x00u);
            break;
        }
        case GritTarget::kFme7:
            pokeExp(0xC000, (std::uint32_t) (8 + t.ch));
            pokeExp(0xE000, 0x00);
            break;
        case GritTarget::kVrc7:
            pokeExp(0x9010, (std::uint32_t) (0x20 + t.ch));
            pokeExp(0x9030,
                    (std::uint32_t) std::max(0, voices_[(size_t) v].lastHi & ~0x10));
            break;
    }
}

std::vector<std::uint8_t> Grit::encodeDpcm(const float* mono, int count,
                                           double sourceRate) {
    std::vector<std::uint8_t> out;
    if (mono == nullptr || count <= 0 || sourceRate <= 0.0) return out;
    const double step = sourceRate / (double) kDpcmRate;
    const int samples = std::min((int) (count / step), 4081 * 8);
    int state = 64;
    std::uint8_t byte = 0;
    for (int i = 0; i < samples; ++i) {
        const double pos = i * step;
        const int i0 = std::min((int) pos, count - 1);
        const int i1 = std::min(i0 + 1, count - 1);
        const double fr = pos - i0;
        const float s = (float) (mono[i0] * (1.0 - fr) + mono[i1] * fr);
        const int want = std::clamp((int) std::lround((s * 0.5 + 0.5) * 127.0), 0, 127);
        const int bit = want > state ? 1 : 0;
        state = std::clamp(state + (bit != 0 ? 2 : -2), 0, 127);
        byte = (std::uint8_t) (byte | (bit << (i & 7)));
        if ((i & 7) == 7) {
            out.push_back(byte);
            byte = 0;
        }
    }
    if (out.size() % 16 != 1) out.resize(out.size() - (out.size() % 16) + 1, 0x55);
    return out;
}

void Grit::loadSample() {
    const auto* sp = params.byName("Sample");
    const std::string path = sp != nullptr ? sp->text : std::string();
    cachedSample_ = path;
    impl_->rom.bytes.clear();
    if (path.empty()) return;
    juce::AudioBuffer<float> buf;
    SoundFileInfo info;
    if (!loadSoundFile(path, buf, info) || buf.getNumSamples() <= 0) return;
    std::vector<float> mono((size_t) buf.getNumSamples(), 0.0f);
    for (int c = 0; c < buf.getNumChannels(); ++c) {
        const float* src = buf.getReadPointer(c);
        for (int i = 0; i < buf.getNumSamples(); ++i)
            mono[(size_t) i] += src[i] / (float) buf.getNumChannels();
    }
    impl_->rom.bytes = encodeDpcm(mono.data(), (int) mono.size(),
                            info.sampleRate > 0.0 ? info.sampleRate : 44100.0);
}

}
