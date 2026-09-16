// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace hum::mov {

using Bytes = std::vector<std::uint8_t>;

inline void u8(Bytes& b, int v) { b.push_back((std::uint8_t) (v & 0xFF)); }
inline void u16(Bytes& b, unsigned v) {
    u8(b, (int) (v >> 8));
    u8(b, (int) v);
}
inline void u32(Bytes& b, std::uint32_t v) {
    u16(b, (unsigned) (v >> 16));
    u16(b, (unsigned) (v & 0xFFFF));
}
inline void tag(Bytes& b, const char* s) {
    for (int i = 0; i < 4; ++i) u8(b, (std::uint8_t) s[i]);
}
inline void raw(Bytes& b, const Bytes& x) { b.insert(b.end(), x.begin(), x.end()); }
inline void zeros(Bytes& b, int n) {
    for (int i = 0; i < n; ++i) u8(b, 0);
}

inline Bytes box(const char* type, const Bytes& payload) {
    Bytes b;
    u32(b, (std::uint32_t) payload.size() + 8);
    tag(b, type);
    raw(b, payload);
    return b;
}

inline Bytes fullBox(const char* type, std::uint32_t flags, const Bytes& payload) {
    Bytes b;
    u32(b, flags & 0x00FFFFFF);
    raw(b, payload);
    return box(type, b);
}

inline void unityMatrix(Bytes& b) {
    u32(b, 0x00010000);
    u32(b, 0);
    u32(b, 0);
    u32(b, 0);
    u32(b, 0x00010000);
    u32(b, 0);
    u32(b, 0);
    u32(b, 0);
    u32(b, 0x40000000);
}

inline constexpr std::uint32_t kMovieScale = 1000;
inline constexpr int kSoundBits = 24;

struct Picture {
    int width = 0, height = 0;
    std::uint32_t timescale = 30000, delta = 1000;
    std::vector<std::uint32_t> sizes;
    std::int64_t offset = 0;
};

struct Sound {
    int channels = 0;
    double sampleRate = 0.0;
    std::uint32_t frames = 0;
    std::int64_t offset = 0;
};

namespace detail {

inline Bytes dataInfo() {
    Bytes dref;
    u32(dref, 1);
    raw(dref, fullBox("url ", 1, {}));
    return box("dinf", fullBox("dref", 0, dref));
}

inline Bytes handler(const char* kind, const char* name) {
    Bytes h;
    tag(h, "mhlr");
    tag(h, kind);
    zeros(h, 12);
    u8(h, (int) std::char_traits<char>::length(name));
    for (const char* c = name; *c != 0; ++c) u8(h, (std::uint8_t) *c);
    return fullBox("hdlr", 0, h);
}

inline Bytes trackHeader(int id, std::uint32_t duration, int width, int height, int volume) {
    Bytes t;
    u32(t, 0);
    u32(t, 0);
    u32(t, (std::uint32_t) id);
    u32(t, 0);
    u32(t, duration);
    zeros(t, 8);
    u16(t, 0);
    u16(t, 0);
    u16(t, (unsigned) volume);
    u16(t, 0);
    unityMatrix(t);
    u32(t, (std::uint32_t) width << 16);
    u32(t, (std::uint32_t) height << 16);
    return fullBox("tkhd", 0x000007, t);
}

inline Bytes mediaHeader(std::uint32_t timescale, std::uint32_t duration) {
    Bytes m;
    u32(m, 0);
    u32(m, 0);
    u32(m, timescale);
    u32(m, duration);
    u16(m, 0x55C4);
    u16(m, 0);
    return fullBox("mdhd", 0, m);
}

inline Bytes pictureEntry(int width, int height) {
    Bytes d;
    u32(d, 86);
    tag(d, "Hap1");
    zeros(d, 6);
    u16(d, 1);
    u16(d, 0);
    u16(d, 0);
    tag(d, "Hap1");
    u32(d, 512);
    u32(d, 512);
    u16(d, (unsigned) width);
    u16(d, (unsigned) height);
    u32(d, 0x00480000);
    u32(d, 0x00480000);
    u32(d, 0);
    u16(d, 1);
    u8(d, 3);
    u8(d, 'H');
    u8(d, 'a');
    u8(d, 'p');
    zeros(d, 28);
    u16(d, 24);
    u16(d, 0xFFFF);
    return d;
}

inline Bytes soundEntry(int channels, double sampleRate) {
    Bytes d;
    u32(d, 36);
    tag(d, "in24");
    zeros(d, 6);
    u16(d, 1);
    u16(d, 0);
    u16(d, 0);
    u32(d, 0);
    u16(d, (unsigned) channels);
    u16(d, kSoundBits);
    u16(d, 0);
    u16(d, 0);
    u32(d, (std::uint32_t) std::llround(sampleRate) << 16);
    return d;
}

inline Bytes sampleTable(const Bytes& entry, std::uint32_t count, std::uint32_t delta,
                         std::uint32_t uniformSize, const std::vector<std::uint32_t>& sizes,
                         std::int64_t offset) {
    Bytes described;
    u32(described, 1);
    raw(described, entry);

    Bytes times;
    u32(times, 1);
    u32(times, count);
    u32(times, delta);

    Bytes runs;
    u32(runs, 1);
    u32(runs, 1);
    u32(runs, count);
    u32(runs, 1);

    Bytes lengths;
    u32(lengths, uniformSize);
    u32(lengths, count);
    if (uniformSize == 0)
        for (auto s : sizes) u32(lengths, s);

    Bytes chunks;
    u32(chunks, 1);
    u32(chunks, (std::uint32_t) offset);

    Bytes stbl;
    raw(stbl, fullBox("stsd", 0, described));
    raw(stbl, fullBox("stts", 0, times));
    raw(stbl, fullBox("stsc", 0, runs));
    raw(stbl, fullBox("stsz", 0, lengths));
    raw(stbl, fullBox("stco", 0, chunks));
    return box("stbl", stbl);
}

inline std::uint32_t movieTicks(double seconds) {
    return (std::uint32_t) std::max(0LL, std::llround(seconds * (double) kMovieScale));
}

inline Bytes pictureTrack(const Picture& p) {
    const auto count = (std::uint32_t) p.sizes.size();
    const double seconds = p.timescale > 0 ? (double) count * p.delta / p.timescale : 0.0;

    Bytes minf;
    Bytes vmhd;
    u16(vmhd, 0);
    zeros(vmhd, 6);
    raw(minf, fullBox("vmhd", 1, vmhd));
    raw(minf, dataInfo());
    raw(minf, sampleTable(pictureEntry(p.width, p.height), count, p.delta, 0, p.sizes, p.offset));

    Bytes mdia;
    raw(mdia, mediaHeader(p.timescale, count * p.delta));
    raw(mdia, handler("vide", "Hap"));
    raw(mdia, box("minf", minf));

    Bytes trak;
    raw(trak, trackHeader(1, movieTicks(seconds), p.width, p.height, 0));
    raw(trak, box("mdia", mdia));
    return box("trak", trak);
}

inline Bytes soundTrack(const Sound& s) {
    const std::uint32_t rate = (std::uint32_t) std::max(1LL, std::llround(s.sampleRate));
    const double seconds = (double) s.frames / (double) rate;

    Bytes minf;
    Bytes smhd;
    u16(smhd, 0);
    u16(smhd, 0);
    raw(minf, fullBox("smhd", 0, smhd));
    raw(minf, dataInfo());
    raw(minf, sampleTable(soundEntry(s.channels, s.sampleRate), s.frames, 1,
                          (std::uint32_t) (s.channels * (kSoundBits / 8)), {}, s.offset));

    Bytes mdia;
    raw(mdia, mediaHeader(rate, s.frames));
    raw(mdia, handler("soun", "Sound"));
    raw(mdia, box("minf", minf));

    Bytes trak;
    raw(trak, trackHeader(2, movieTicks(seconds), 0, 0, 0x0100));
    raw(trak, box("mdia", mdia));
    return box("trak", trak);
}

}

inline Bytes movieHeader(const Picture& p, const Sound& s) {
    const auto count = (std::uint32_t) p.sizes.size();
    const double picture = p.timescale > 0 ? (double) count * p.delta / p.timescale : 0.0;
    const double sound = s.channels > 0 && s.sampleRate > 0.0 ? (double) s.frames / s.sampleRate
                                                              : 0.0;
    Bytes mvhd;
    u32(mvhd, 0);
    u32(mvhd, 0);
    u32(mvhd, kMovieScale);
    u32(mvhd, detail::movieTicks(std::max(picture, sound)));
    u32(mvhd, 0x00010000);
    u16(mvhd, 0x0100);
    zeros(mvhd, 10);
    unityMatrix(mvhd);
    zeros(mvhd, 24);
    u32(mvhd, s.channels > 0 ? 3 : 2);

    Bytes moov;
    raw(moov, fullBox("mvhd", 0, mvhd));
    raw(moov, detail::pictureTrack(p));
    if (s.channels > 0 && s.frames > 0) raw(moov, detail::soundTrack(s));
    return box("moov", moov);
}

inline void interleave24(const float* const* channels, int count, int frames, Bytes& out) {
    out.clear();
    out.reserve((size_t) frames * (size_t) count * 3u);
    for (int f = 0; f < frames; ++f)
        for (int c = 0; c < count; ++c) {
            const float v = channels[c][f];
            const double clamped = v > 1.0f ? 1.0 : (v < -1.0f ? -1.0 : (double) v);
            const std::int32_t s = (std::int32_t) std::llround(clamped * 8388607.0);
            out.push_back((std::uint8_t) ((s >> 16) & 0xFF));
            out.push_back((std::uint8_t) ((s >> 8) & 0xFF));
            out.push_back((std::uint8_t) (s & 0xFF));
        }
}

}
