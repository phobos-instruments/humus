// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>

#include "hum/dsp/DspMath.h"

namespace hum {

struct RexData {
    bool parsed = false;
    bool hasAudio = false;
    double tempoBpm = 0.0;
    int bars = 0, beatsPerBar = 0;
    double sampleRate = 0.0;
    int totalSamples = 0;
    std::vector<int> slices;
    juce::AudioBuffer<float> audio;
};

inline bool isRexPath(std::string uri) {
    if (uri.rfind("file://", 0) == 0) uri = uri.substr(7);
    const auto ext = juce::File(juce::String(juce::CharPointer_UTF8(uri.c_str())))
                         .getFileExtension().toLowerCase();
    return ext == ".rx2" || ext == ".rex";
}

inline RexData loadRex1(const std::uint8_t* d, std::size_t n) {
    RexData out;
    auto u32 = [&](std::size_t o) -> std::uint32_t {
        return o + 4 <= n ? (std::uint32_t) ((d[o] << 24) | (d[o + 1] << 16)
                                             | (d[o + 2] << 8) | d[o + 3]) : 0u;
    };
    auto u16 = [&](std::size_t o) -> std::uint16_t {
        return o + 2 <= n ? (std::uint16_t) ((d[o] << 8) | d[o + 1]) : (std::uint16_t) 0;
    };
    int channels = 0, frames = 0, bits = 0;
    double rate = 0.0;
    std::size_t sndOff = 0, sndBytes = 0, rexOff = 0, rexSize = 0;
    for (std::size_t off = 12; off + 8 <= n;) {
        const std::string id((const char*) d + off, 4);
        const std::size_t size = u32(off + 4);
        const std::size_t body = off + 8;
        if (body + size > n) break;
        if (id == "COMM" && size >= 18) {
            channels = u16(body);
            frames = (int) u32(body + 2);
            bits = u16(body + 6);
            const int exp = (int) (u16(body + 8) & 0x7fff) - 16383 - 63;
            const double mant = (double) (((std::uint64_t) u32(body + 10) << 32)
                                          | u32(body + 14));
            rate = std::ldexp(mant, exp);
        } else if (id == "SSND" && size >= 8) {
            sndOff = body + 8 + u32(body);
            sndBytes = size - 8 - u32(body);
        } else if (id == "APPL" && size >= 4
                   && std::string((const char*) d + body, 4) == "REX ") {
            rexOff = body + 4;
            rexSize = size - 4;
        }
        off = body + size;
        if (off & 1) ++off;
    }
    if (channels < 1 || frames <= 0 || bits != 16 || rate < 8000.0
        || sndOff == 0 || rexOff == 0)
        return out;

    const int ppqBar = u16(rexOff + 8);
    const double bpm = u32(rexOff + 12) / 1000.0;
    if (ppqBar <= 0 || bpm < 20.0 || bpm > 999.0) return out;

    struct Row { int start, len, tick; };
    std::vector<Row> rows;
    for (std::size_t o = 16; o + 24 <= rexSize && rows.empty(); o += 4) {
        if (u32(rexOff + o) != 0 || u32(rexOff + o + 8) != 0) continue;
        const int len0 = (int) u32(rexOff + o + 4);
        const int s1 = (int) u32(rexOff + o + 12);
        if (len0 <= 0 || len0 > frames || s1 <= 0 || s1 > frames) continue;
        for (std::size_t p = o; p + 12 <= rexSize;) {
            const Row r{(int) u32(rexOff + p), (int) u32(rexOff + p + 4),
                        (int) u32(rexOff + p + 8)};
            if (r.len <= 0 || (std::int64_t) r.start + r.len > frames
                || (!rows.empty() && (r.start <= rows.back().start
                                      || r.tick <= rows.back().tick)))
                break;
            rows.push_back(r);
            p += 12;
        }
        if (rows.size() < 2) rows.clear();
    }
    if (rows.empty()) return out;

    const int bars = (rows.back().tick / ppqBar) + 1;
    const double quarters = (double) (bars * ppqBar) / ((double) ppqBar / 4.0);
    const int loopLen = (int) std::lround(quarters * kSecondsPerMinute / bpm * rate);
    if (loopLen <= 0 || loopLen > 60 * 48000 * 8) return out;
    out.audio.setSize(channels, loopLen);
    out.audio.clear();
    for (const auto& r : rows) {
        const int dst = (int) std::lround((double) r.tick / (double) (ppqBar * bars)
                                          * (double) loopLen);
        out.slices.push_back(dst);
        const int count = std::min(r.len, loopLen - dst);
        for (int c = 0; c < channels; ++c)
            for (int i = 0; i < count; ++i) {
                const std::size_t o = sndOff
                    + (std::size_t) (r.start + i) * (std::size_t) channels * 2
                    + (std::size_t) c * 2;
                if (o + 1 >= n) break;
                const auto v = (std::int16_t) ((d[o] << 8) | d[o + 1]);
                out.audio.addSample(c, dst + i, (float) (v / 32768.0));
            }
    }
    juce::ignoreUnused(sndBytes);
    out.parsed = true;
    out.hasAudio = true;
    out.tempoBpm = bpm;
    out.bars = bars;
    out.beatsPerBar = 4;
    out.sampleRate = rate;
    out.totalSamples = loopLen;
    return out;
}

inline RexData loadRexFile(const juce::File& f) {
    RexData out;
    juce::MemoryBlock mb;
    if (!f.loadFileAsData(mb)) return out;
    const auto* d = static_cast<const std::uint8_t*>(mb.getData());
    const std::size_t n = mb.getSize();
    if (n >= 12 && std::string((const char*) d, 4) == "FORM"
        && std::string((const char*) d + 8, 4) == "AIFF")
        return loadRex1(d, n);
    auto u32 = [&](std::size_t o) -> std::uint32_t {
        return o + 4 <= n ? (std::uint32_t) ((d[o] << 24) | (d[o + 1] << 16)
                                             | (d[o + 2] << 8) | d[o + 3]) : 0u;
    };
    auto u16 = [&](std::size_t o) -> std::uint16_t {
        return o + 2 <= n ? (std::uint16_t) ((d[o] << 8) | d[o + 1]) : (std::uint16_t) 0;
    };
    auto tag = [&](std::size_t o) {
        return o + 4 <= n ? std::string((const char*) d + o, 4) : std::string();
    };
    if (n < 12 || tag(0) != "CAT " || tag(8) != "REX2") return out;

    std::size_t sdatOff = 0, sdatSize = 0;
    std::function<void(std::size_t, std::size_t)> walk = [&](std::size_t off,
                                                             std::size_t end) {
        while (off + 8 <= end && off + 8 <= n) {
            const std::string id = tag(off);
            const std::size_t size = u32(off + 4);
            const std::size_t body = off + 8;
            if (body + size > n) return;
            if (id == "CAT ") {
                walk(body + 4, body + size);
            } else if (id == "GLOB" && size >= 20) {
                out.bars = u16(body + 4);
                out.beatsPerBar = u16(body + 6);
                const double bpm = u32(body + 16) / 1000.0;
                if (bpm >= 20.0 && bpm <= 999.0) out.tempoBpm = bpm;
            } else if (id == "SINF" && size >= 10) {
                out.sampleRate = u32(body + 2);
                out.totalSamples = (int) u32(body + 6);
            } else if (id == "SLCE" && size >= 4) {
                out.slices.push_back((int) u32(body));
            } else if (id == "SDAT") {
                sdatOff = body;
                sdatSize = size;
            }
            off = body + size;
            if (off & 1) ++off;
        }
    };
    walk(12, n);

    out.parsed = out.totalSamples > 0 && !out.slices.empty();
    if (!out.parsed) return out;

    for (int ch = 1; ch <= 2 && !out.hasAudio; ++ch)
        for (int bytes = 2; bytes <= 3 && !out.hasAudio; ++bytes) {
            if (sdatSize != (std::size_t) out.totalSamples * (std::size_t) ch
                                * (std::size_t) bytes)
                continue;
            out.audio.setSize(ch, out.totalSamples);
            const double norm = bytes == 2 ? 1.0 / 32768.0 : 1.0 / 8388608.0;
            for (int i = 0; i < out.totalSamples; ++i)
                for (int c = 0; c < ch; ++c) {
                    const std::size_t o = sdatOff
                        + (std::size_t) (i * ch + c) * (std::size_t) bytes;
                    std::int32_t v = bytes == 2
                        ? (std::int32_t) (std::int16_t) ((d[o] << 8) | d[o + 1])
                        : ((std::int32_t) ((d[o] << 24) | (d[o + 1] << 16)
                                           | (d[o + 2] << 8)) >> 8);
                    out.audio.setSample(c, i, (float) (v * norm));
                }
            out.hasAudio = true;
        }
    return out;
}

}
