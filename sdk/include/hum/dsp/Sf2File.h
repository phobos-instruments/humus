// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include <juce_core/juce_core.h>

#include "hum/dsp/DspMath.h"

namespace hum::sf2 {

enum Gen : std::uint16_t {
    kStartAddrsOffset = 0, kEndAddrsOffset = 1,
    kStartLoopOffset = 2, kEndLoopOffset = 3,
    kStartAddrsCoarse = 4, kEndAddrsCoarse = 12,
    kStartLoopCoarse = 45, kEndLoopCoarse = 50,
    kInitialFilterFc = 8, kInitialFilterQ = 9,
    kPan = 17,
    kDelayVolEnv = 33, kAttackVolEnv = 34, kHoldVolEnv = 35,
    kDecayVolEnv = 36, kSustainVolEnv = 37, kReleaseVolEnv = 38,
    kInstrument = 41, kKeyRange = 43, kVelRange = 44,
    kInitialAttenuation = 48,
    kCoarseTune = 51, kFineTune = 52,
    kSampleID = 53, kSampleModes = 54, kScaleTuning = 56,
    kOverridingRootKey = 58,
    kGenCount = 60,
};

struct Sample {
    std::string name;
    std::uint32_t start = 0, end = 0, loopStart = 0, loopEnd = 0;
    std::uint32_t sampleRate = (int) kDefaultSampleRate;
    int originalKey = 60;
    int correction = 0;
    std::uint16_t link = 0;
    std::uint16_t type = 1;
};

struct Zone {
    int sampleIndex = -1;
    int keyLo = 0, keyHi = kMidiMax, velLo = 0, velHi = kMidiMax;
    int rootKey = -1;
    int coarseTune = 0, fineTune = 0, scaleTuning = 100;
    int sampleModes = 0;
    double attenuationCb = 0.0;
    double panTenthPct = 0.0;
    double filterFc = 13500.0;
    double filterQ = 0.0;
    double delayTc = -12000.0, attackTc = -12000.0, holdTc = -12000.0;
    double decayTc = -12000.0, sustainCb = 0.0, releaseTc = -12000.0;
    std::int32_t startOffset = 0, endOffset = 0, loopStartOffset = 0, loopEndOffset = 0;
};

struct Preset {
    std::string name;
    int bank = 0, program = 0;
    std::vector<Zone> zones;
};

struct Sf2Data {
    bool parsed = false;
    std::string name;
    std::vector<std::int16_t> pcm;
    std::vector<Sample> samples;
    std::vector<Preset> presets;
};

inline double centsToHz(double cents) { return 8.176 * std::pow(2.0, cents / 1200.0); }
inline double centibelsToGain(double cb) { return std::pow(10.0, -cb / 200.0); }
inline double timecentsToSeconds(double tc) { return std::pow(2.0, tc / 1200.0); }

inline bool isSf2Path(std::string uri) {
    if (uri.rfind("file://", 0) == 0) uri = uri.substr(7);
    return juce::File(juce::String(juce::CharPointer_UTF8(uri.c_str())))
               .getFileExtension().toLowerCase() == ".sf2";
}

namespace detail {

struct GenSet {
    std::array<std::int32_t, kGenCount> v{};
    std::array<bool, kGenCount> set{};
    void put(std::uint16_t op, std::int32_t amount) {
        if (op < kGenCount) { v[op] = amount; set[op] = true; }
    }
};

struct Reader {
    const std::uint8_t* d; std::size_t n;
    std::uint16_t u16(std::size_t o) const {
        return o + 2 <= n ? (std::uint16_t) (d[o] | (d[o + 1] << 8)) : (std::uint16_t) 0;
    }
    std::int16_t s16(std::size_t o) const { return (std::int16_t) u16(o); }
    std::uint32_t u32(std::size_t o) const {
        return o + 4 <= n ? (std::uint32_t) (d[o] | (d[o + 1] << 8)
                                             | (d[o + 2] << 16) | ((std::uint32_t) d[o + 3] << 24))
                          : 0u;
    }
    std::string fourcc(std::size_t o) const {
        return o + 4 <= n ? std::string((const char*) d + o, 4) : std::string();
    }
    std::string name(std::size_t o, std::size_t len) const {
        if (o + len > n) return {};
        std::size_t k = 0;
        while (k < len && d[o + k] != 0) ++k;
        return std::string((const char*) d + o, k);
    }
};

struct Span { std::size_t off = 0, size = 0; };

inline Span findChunk(const Reader& r, std::size_t from, std::size_t to,
                      const std::string& id) {
    for (std::size_t o = from; o + 8 <= to;) {
        const std::size_t size = r.u32(o + 4);
        const std::size_t body = o + 8;
        if (body + size > to) break;
        if (r.fourcc(o) == id) return {body, size};
        o = body + size + (size & 1);
    }
    return {};
}

inline Span findList(const Reader& r, const std::string& type) {
    for (std::size_t o = 12; o + 12 <= r.n;) {
        const std::size_t size = r.u32(o + 4);
        const std::size_t body = o + 8;
        if (body + size > r.n) break;
        if (r.fourcc(o) == "LIST" && r.fourcc(body) == type)
            return {body + 4, size >= 4 ? size - 4 : 0};
        o = body + size + (size & 1);
    }
    return {};
}

inline bool readGens(const Reader& r, Span gen, std::size_t first, std::size_t last,
                     GenSet& out) {
    for (std::size_t i = first; i < last; ++i) {
        const std::size_t o = gen.off + i * 4;
        if (o + 4 > gen.off + gen.size) return false;
        out.put(r.u16(o), (std::int32_t) r.s16(o + 2));
    }
    return true;
}

template <typename T>
struct Ref { std::uint16_t op; T Zone::* member; };

inline constexpr Ref<double> kScalars[] = {
    {kInitialAttenuation, &Zone::attenuationCb}, {kPan, &Zone::panTenthPct},
    {kInitialFilterFc, &Zone::filterFc},         {kInitialFilterQ, &Zone::filterQ},
    {kDelayVolEnv, &Zone::delayTc},              {kAttackVolEnv, &Zone::attackTc},
    {kHoldVolEnv, &Zone::holdTc},                {kDecayVolEnv, &Zone::decayTc},
    {kSustainVolEnv, &Zone::sustainCb},          {kReleaseVolEnv, &Zone::releaseTc},
};
inline constexpr Ref<int> kTunings[] = {
    {kCoarseTune, &Zone::coarseTune}, {kFineTune, &Zone::fineTune},
};

inline void applyAbsolute(const GenSet& g, Zone& z) {
    for (const auto& r : kScalars) if (g.set[r.op]) z.*r.member = g.v[r.op];
    for (const auto& r : kTunings) if (g.set[r.op]) z.*r.member = g.v[r.op];
    if (g.set[kKeyRange]) { z.keyLo = g.v[kKeyRange] & 0xff; z.keyHi = (g.v[kKeyRange] >> 8) & 0xff; }
    if (g.set[kVelRange]) { z.velLo = g.v[kVelRange] & 0xff; z.velHi = (g.v[kVelRange] >> 8) & 0xff; }
    if (g.set[kOverridingRootKey]) z.rootKey = g.v[kOverridingRootKey];
    if (g.set[kScaleTuning]) z.scaleTuning = g.v[kScaleTuning];
    if (g.set[kSampleModes]) z.sampleModes = g.v[kSampleModes] & 3;
    z.startOffset = g.v[kStartAddrsOffset] + g.v[kStartAddrsCoarse] * 32768;
    z.endOffset = g.v[kEndAddrsOffset] + g.v[kEndAddrsCoarse] * 32768;
    z.loopStartOffset = g.v[kStartLoopOffset] + g.v[kStartLoopCoarse] * 32768;
    z.loopEndOffset = g.v[kEndLoopOffset] + g.v[kEndLoopCoarse] * 32768;
}

inline void applyRelative(const GenSet& g, Zone& z) {
    for (const auto& r : kScalars) z.*r.member += g.v[r.op];
    for (const auto& r : kTunings) z.*r.member += g.v[r.op];
    if (g.set[kKeyRange]) {
        z.keyLo = std::max(z.keyLo, g.v[kKeyRange] & 0xff);
        z.keyHi = std::min(z.keyHi, (g.v[kKeyRange] >> 8) & 0xff);
    }
    if (g.set[kVelRange]) {
        z.velLo = std::max(z.velLo, g.v[kVelRange] & 0xff);
        z.velHi = std::min(z.velHi, (g.v[kVelRange] >> 8) & 0xff);
    }
}

}

inline Sf2Data load(const std::uint8_t* data, std::size_t size) {
    using namespace detail;
    Sf2Data out;
    Reader r{data, size};
    if (size < 12 || r.fourcc(0) != "RIFF" || r.fourcc(8) != "sfbk") return out;

    const Span info = findList(r, "INFO");
    const Span sdta = findList(r, "sdta");
    const Span pdta = findList(r, "pdta");
    if (pdta.size == 0) return out;

    if (info.size) {
        const Span nm = findChunk(r, info.off, info.off + info.size, "INAM");
        if (nm.size) out.name = r.name(nm.off, nm.size);
    }

    if (sdta.size) {
        const Span smpl = findChunk(r, sdta.off, sdta.off + sdta.size, "smpl");
        out.pcm.resize(smpl.size / 2);
        for (std::size_t i = 0; i < out.pcm.size(); ++i)
            out.pcm[i] = r.s16(smpl.off + i * 2);
    }

    const std::size_t pEnd = pdta.off + pdta.size;
    const Span phdr = findChunk(r, pdta.off, pEnd, "phdr");
    const Span pbag = findChunk(r, pdta.off, pEnd, "pbag");
    const Span pgen = findChunk(r, pdta.off, pEnd, "pgen");
    const Span inst = findChunk(r, pdta.off, pEnd, "inst");
    const Span ibag = findChunk(r, pdta.off, pEnd, "ibag");
    const Span igen = findChunk(r, pdta.off, pEnd, "igen");
    const Span shdr = findChunk(r, pdta.off, pEnd, "shdr");
    if (phdr.size < 76 || pbag.size < 8 || pgen.size < 8
        || inst.size < 44 || ibag.size < 8 || igen.size < 8 || shdr.size < 92)
        return out;

    const std::size_t nPre = phdr.size / 38 - 1;
    const std::size_t nIns = inst.size / 22 - 1;
    const std::size_t nSmp = shdr.size / 46 - 1;
    const std::size_t nPbag = pbag.size / 4, nIbag = ibag.size / 4;

    out.samples.reserve(nSmp);
    for (std::size_t i = 0; i < nSmp; ++i) {
        const std::size_t o = shdr.off + i * 46;
        Sample s;
        s.name = r.name(o, 20);
        s.start = r.u32(o + 20); s.end = r.u32(o + 24);
        s.loopStart = r.u32(o + 28); s.loopEnd = r.u32(o + 32);
        s.sampleRate = r.u32(o + 36);
        s.originalKey = data[o + 40];
        s.correction = (std::int8_t) data[o + 41];
        s.link = r.u16(o + 42);
        s.type = r.u16(o + 44);
        out.samples.push_back(std::move(s));
    }

    std::vector<std::vector<Zone>> instruments(nIns);
    for (std::size_t i = 0; i < nIns; ++i) {
        const std::size_t bagFirst = r.u16(inst.off + i * 22 + 20);
        const std::size_t bagLast = r.u16(inst.off + (i + 1) * 22 + 20);
        if (bagLast > nIbag || bagFirst > bagLast) return out;
        GenSet global;
        for (std::size_t b = bagFirst; b < bagLast; ++b) {
            const std::size_t g0 = r.u16(ibag.off + b * 4);
            const std::size_t g1 = r.u16(ibag.off + (b + 1) * 4);
            GenSet g = global;
            if (!readGens(r, igen, g0, g1, g)) return out;
            if (!g.set[kSampleID]) {
                if (b == bagFirst) { global = g; global.set[kSampleID] = false; }
                continue;
            }
            Zone z;
            applyAbsolute(g, z);
            z.sampleIndex = g.v[kSampleID];
            if (z.sampleIndex < 0 || (std::size_t) z.sampleIndex >= nSmp) continue;
            instruments[i].push_back(z);
        }
    }

    out.presets.reserve(nPre);
    for (std::size_t p = 0; p < nPre; ++p) {
        const std::size_t o = phdr.off + p * 38;
        Preset pre;
        pre.name = r.name(o, 20);
        pre.program = r.u16(o + 20);
        pre.bank = r.u16(o + 22);
        const std::size_t bagFirst = r.u16(o + 24);
        const std::size_t bagLast = r.u16(phdr.off + (p + 1) * 38 + 24);
        if (bagLast > nPbag || bagFirst > bagLast) return out;

        GenSet global;
        for (std::size_t b = bagFirst; b < bagLast; ++b) {
            const std::size_t g0 = r.u16(pbag.off + b * 4);
            const std::size_t g1 = r.u16(pbag.off + (b + 1) * 4);
            GenSet g = global;
            if (!readGens(r, pgen, g0, g1, g)) return out;
            if (!g.set[kInstrument]) {
                if (b == bagFirst) { global = g; global.set[kInstrument] = false; }
                continue;
            }
            const std::size_t which = (std::size_t) g.v[kInstrument];
            if (which >= nIns) continue;
            for (const Zone& base : instruments[which]) {
                Zone z = base;
                applyRelative(g, z);
                if (z.keyLo > z.keyHi || z.velLo > z.velHi) continue;
                pre.zones.push_back(z);
            }
        }
        out.presets.push_back(std::move(pre));
    }

    out.parsed = !out.presets.empty();
    return out;
}

inline Sf2Data loadFile(const juce::File& f) {
    juce::MemoryBlock mb;
    if (!f.loadFileAsData(mb) || mb.getSize() < 12) return {};
    return load((const std::uint8_t*) mb.getData(), mb.getSize());
}

}
