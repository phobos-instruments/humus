// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include <juce_core/juce_core.h>

#include "core/video/HapFile.h"

namespace hum::media {

struct Picture {
    std::string tag, codec, profile;
    int width = 0, height = 0;
    double fps = 0.0, seconds = 0.0;
    std::int64_t frames = 0, keyframes = 0, bytes = 0;
};

struct Sound {
    std::string tag, codec;
    double sampleRate = 0.0, seconds = 0.0;
    int channels = 0, bits = 0;
    std::int64_t samples = 0, bytes = 0;
};

struct Movie {
    bool ok = false;
    std::string brand;
    double seconds = 0.0;
    std::vector<Picture> pictures;
    std::vector<Sound> sounds;
};

inline std::string tagText(std::uint32_t t) {
    std::string s;
    for (int shift = 24; shift >= 0; shift -= 8) {
        const char c = (char) ((t >> shift) & 0xFF);
        s.push_back(c >= 0x20 && c < 0x7F ? c : '?');
    }
    return s;
}

inline std::string codecName(const std::string& tag) {
    struct Name { const char* tag; const char* name; };
    static const Name names[] = {
        {"avc1", "H.264"},        {"avc3", "H.264"},        {"hvc1", "H.265"},
        {"hev1", "H.265"},        {"vp09", "VP9"},          {"av01", "AV1"},
        {"mp4v", "MPEG-4 Part 2"}, {"jpeg", "Motion JPEG"}, {"mjpa", "Motion JPEG"},
        {"Hap1", "HAP"},          {"Hap5", "HAP Alpha"},    {"HapY", "HAP Q"},
        {"HapM", "HAP Q Alpha"},  {"HapA", "HAP Alpha Only"},
        {"mp4a", "AAC"},          {"Opus", "Opus"},         {"fLaC", "FLAC"},
        {".mp3", "MP3"},          {"in24", "PCM 24-bit"},   {"in32", "PCM 32-bit"},
        {"sowt", "PCM 16-bit"},   {"twos", "PCM 16-bit"},   {"lpcm", "PCM"},
        {"raw ", "PCM 8-bit"},    {"fl32", "PCM float"},    {"fl64", "PCM double"},
        {"alac", "Lossless"},     {"ulaw", "u-law"},        {"alaw", "A-law"},
    };
    for (const auto& n : names)
        if (tag == n.tag) return n.name;
    return {};
}

inline std::string h264Profile(int idc, int level) {
    std::string p;
    switch (idc) {
        case 66: p = "Baseline"; break;
        case 77: p = "Main"; break;
        case 88: p = "Extended"; break;
        case 100: p = "High"; break;
        case 110: p = "High 10"; break;
        case 122: p = "High 4:2:2"; break;
        case 244: p = "High 4:4:4"; break;
        default: return {};
    }
    if (level > 0) {
        p += " " + std::to_string(level / 10);
        if (level % 10 != 0) p += "." + std::to_string(level % 10);
    }
    return p;
}

inline bool everyFrameStandsAlone(const Picture& p) {
    return p.frames > 0 && p.keyframes >= p.frames;
}

namespace detail {

using hap::detail::be16;
using hap::detail::be32;
using hap::detail::Box;
using hap::detail::fourcc;
using hap::detail::nextBox;
using hap::detail::slurp;

struct Track {
    std::uint32_t handler = 0;
    Picture pic;
    Sound snd;
    std::uint32_t timescale = 0, commonDelta = 0, commonRun = 0;
    std::uint64_t duration = 0, ticks = 0;
    std::int64_t frames = 0, bytes = 0, sync = -1;
};

inline void childBoxes(const std::uint8_t* p, size_t n, size_t from, Track& t) {
    size_t at = from;
    while (at + 8 <= n) {
        const std::uint32_t size = be32(p + at);
        const std::uint32_t type = be32(p + at + 4);
        if (size < 8 || at + size > n) break;
        if (type == fourcc("avcC") && size >= 12)
            t.pic.profile = h264Profile(p[at + 9], p[at + 11]);
        at += size;
    }
}

inline void parseStsd(const std::uint8_t* p, size_t n, Track& t) {
    if (n < 16) return;
    const auto* e = p + 8;
    const size_t en = n - 8;
    const std::uint32_t size = be32(e);
    const auto tag = tagText(be32(e + 4));
    if (t.handler == fourcc("vide")) {
        t.pic.tag = tag;
        t.pic.codec = codecName(tag);
        if (en >= 36) {
            t.pic.width = be16(e + 32);
            t.pic.height = be16(e + 34);
        }
        if (size >= 86 && size <= en) childBoxes(e, size, 86, t);
    } else if (t.handler == fourcc("soun")) {
        t.snd.tag = tag;
        t.snd.codec = codecName(tag);
        if (en < 36) return;
        const int version = be16(e + 16);
        if (version == 2 && en >= 60) {
            double rate = 0.0;
            std::uint8_t be[8];
            for (int i = 0; i < 8; ++i) be[i] = e[47 - i];
            std::memcpy(&rate, be, 8);
            t.snd.sampleRate = rate;
            t.snd.channels = (int) be32(e + 48);
            t.snd.bits = (int) be32(e + 56);
        } else {
            t.snd.channels = be16(e + 24);
            t.snd.bits = be16(e + 26);
            t.snd.sampleRate = (double) (be32(e + 32) >> 16);
        }
    }
}

inline void parseStbl(juce::InputStream& in, const Box& stbl, Track& t) {
    in.setPosition(stbl.payload);
    const auto end = stbl.payload + stbl.payloadSize;
    Box b;
    while (in.getPosition() < end && nextBox(in, end, b)) {
        const auto next = b.payload + b.payloadSize;
        const auto body = slurp(in, b);
        const auto* p = body.data();
        const auto n = body.size();
        if (b.type == fourcc("stsd")) {
            parseStsd(p, n, t);
        } else if (b.type == fourcc("stts") && n >= 8) {
            const std::uint32_t count = be32(p + 4);
            for (std::uint32_t i = 0; i < count && 8 + 8 * (size_t) i + 8 <= n; ++i) {
                const std::uint32_t run = be32(p + 8 + 8 * (size_t) i);
                const std::uint32_t delta = be32(p + 12 + 8 * (size_t) i);
                t.frames += run;
                t.ticks += (std::uint64_t) run * delta;
                if (run > t.commonRun) {
                    t.commonRun = run;
                    t.commonDelta = delta;
                }
            }
        } else if (b.type == fourcc("stss") && n >= 8) {
            t.sync = be32(p + 4);
        } else if (b.type == fourcc("stsz") && n >= 12) {
            const std::uint32_t uniform = be32(p + 4);
            const std::uint32_t count = be32(p + 8);
            if (uniform != 0) t.bytes = (std::int64_t) uniform * count;
            else
                for (std::uint32_t i = 0; i < count && 12 + 4 * (size_t) i + 4 <= n; ++i)
                    t.bytes += be32(p + 12 + 4 * (size_t) i);
        }
        in.setPosition(next);
    }
}

inline void parseTrak(juce::InputStream& in, const Box& trak, Track& t) {
    in.setPosition(trak.payload);
    const auto end = trak.payload + trak.payloadSize;
    Box b;
    while (in.getPosition() < end && nextBox(in, end, b)) {
        const auto next = b.payload + b.payloadSize;
        if (b.type == fourcc("mdia") || b.type == fourcc("minf")) {
            parseTrak(in, b, t);
        } else if (b.type == fourcc("mdhd")) {
            const auto body = slurp(in, b);
            const auto* p = body.data();
            if (body.size() >= 24 && p[0] == 0) {
                t.timescale = be32(p + 12);
                t.duration = be32(p + 16);
            } else if (body.size() >= 36 && p[0] == 1) {
                t.timescale = be32(p + 20);
                t.duration = ((std::uint64_t) be32(p + 24) << 32) | be32(p + 28);
            }
        } else if (b.type == fourcc("hdlr")) {
            const auto body = slurp(in, b);
            if (body.size() >= 12) t.handler = be32(body.data() + 8);
        } else if (b.type == fourcc("stbl")) {
            parseStbl(in, b, t);
        }
        in.setPosition(next);
    }
}

inline void finish(Track& t, Movie& m) {
    const double scale = t.timescale > 0 ? (double) t.timescale : 0.0;
    const double seconds = scale > 0.0 ? (double) (t.ticks > 0 ? t.ticks : t.duration) / scale : 0.0;
    if (t.handler == fourcc("vide")) {
        auto& p = t.pic;
        p.frames = t.frames;
        p.seconds = seconds;
        p.fps = t.commonDelta > 0 && scale > 0.0 ? scale / (double) t.commonDelta
                : seconds > 0.0 ? (double) t.frames / seconds : 0.0;
        p.keyframes = t.sync >= 0 ? t.sync : t.frames;
        p.bytes = t.bytes;
        m.pictures.push_back(p);
    } else if (t.handler == fourcc("soun")) {
        auto& s = t.snd;
        s.samples = t.frames;
        s.seconds = seconds;
        s.bytes = t.bytes;
        if (s.sampleRate <= 0.0 && seconds > 0.0) s.sampleRate = (double) t.frames / seconds;
        m.sounds.push_back(s);
    }
}

}

inline Movie probeMovie(const juce::File& file) {
    using namespace detail;
    Movie m;
    juce::FileInputStream in(file);
    if (!in.openedOk()) return m;
    const auto end = in.getTotalLength();
    Box b;
    bool sawMoov = false;
    while (in.getPosition() < end && nextBox(in, end, b)) {
        const auto next = b.payload + b.payloadSize;
        if (b.type == fourcc("ftyp")) {
            const auto body = slurp(in, b);
            if (body.size() >= 4) m.brand = tagText(be32(body.data()));
        } else if (b.type == fourcc("moov")) {
            sawMoov = true;
            in.setPosition(b.payload);
            const auto moovEnd = b.payload + b.payloadSize;
            Box inner;
            while (in.getPosition() < moovEnd && nextBox(in, moovEnd, inner)) {
                const auto innerNext = inner.payload + inner.payloadSize;
                if (inner.type == fourcc("mvhd")) {
                    const auto body = slurp(in, inner);
                    const auto* p = body.data();
                    if (body.size() >= 20 && p[0] == 0 && be32(p + 12) > 0)
                        m.seconds = (double) be32(p + 16) / (double) be32(p + 12);
                    else if (body.size() >= 32 && p[0] == 1 && be32(p + 20) > 0)
                        m.seconds = (double) (((std::uint64_t) be32(p + 24) << 32) | be32(p + 28))
                                    / (double) be32(p + 20);
                } else if (inner.type == fourcc("trak")) {
                    Track t;
                    parseTrak(in, inner, t);
                    finish(t, m);
                }
                in.setPosition(innerNext);
            }
        }
        in.setPosition(next);
    }
    m.ok = sawMoov && (!m.pictures.empty() || !m.sounds.empty());
    return m;
}

}
