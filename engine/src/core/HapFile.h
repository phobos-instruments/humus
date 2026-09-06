#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <juce_core/juce_core.h>

#include "core/SnappyDecode.h"

namespace hum::hap {

enum class Tex { None, DXT1, DXT5, YCoCgDXT5 };

struct Sample {
    std::int64_t offset = 0;
    std::uint32_t size = 0;
};

struct Movie {
    bool ok = false;
    int width = 0, height = 0;
    double fps = 30.0;
    std::vector<Sample> samples;
};

namespace detail {

inline std::uint32_t be32(const std::uint8_t* p) {
    return ((std::uint32_t) p[0] << 24) | ((std::uint32_t) p[1] << 16)
         | ((std::uint32_t) p[2] << 8) | (std::uint32_t) p[3];
}
inline std::uint16_t be16(const std::uint8_t* p) {
    return (std::uint16_t) (((std::uint16_t) p[0] << 8) | p[1]);
}
inline std::uint32_t le32(const std::uint8_t* p) {
    return (std::uint32_t) p[0] | ((std::uint32_t) p[1] << 8)
         | ((std::uint32_t) p[2] << 16) | ((std::uint32_t) p[3] << 24);
}

struct Box {
    std::uint32_t type = 0;
    std::int64_t payload = 0, payloadSize = 0;
};

inline bool nextBox(juce::InputStream& in, std::int64_t end, Box& out) {
    const auto at = in.getPosition();
    if (at + 8 > end) return false;
    std::uint8_t hdr[8];
    if (in.read(hdr, 8) != 8) return false;
    std::int64_t size = be32(hdr);
    out.type = be32(hdr + 4);
    out.payload = at + 8;
    if (size == 1) {
        std::uint8_t big[8];
        if (in.read(big, 8) != 8) return false;
        size = (std::int64_t) (((std::uint64_t) be32(big) << 32) | be32(big + 4));
        out.payload = at + 16;
    } else if (size == 0) {
        size = end - at;
    }
    if (size < 8 || at + size > end) return false;
    out.payloadSize = at + size - out.payload;
    return true;
}

constexpr std::uint32_t fourcc(const char (&s)[5]) {
    return ((std::uint32_t) (std::uint8_t) s[0] << 24)
         | ((std::uint32_t) (std::uint8_t) s[1] << 16)
         | ((std::uint32_t) (std::uint8_t) s[2] << 8) | (std::uint32_t) (std::uint8_t) s[3];
}

struct TrackTables {
    bool isHap = false;
    int width = 0, height = 0;
    std::uint32_t timescale = 0, delta = 0;
    std::vector<std::uint32_t> sizes;
    std::vector<std::int64_t> chunkOffsets;
    struct ChunkRun { std::uint32_t firstChunk = 0, perChunk = 0; };
    std::vector<ChunkRun> runs;
};

inline std::vector<std::uint8_t> slurp(juce::InputStream& in, const Box& b) {
    std::vector<std::uint8_t> out((size_t) b.payloadSize);
    in.setPosition(b.payload);
    if (in.read(out.data(), (int) out.size()) != (int) out.size()) out.clear();
    return out;
}

inline void parseStbl(juce::InputStream& in, const Box& stbl, TrackTables& t) {
    in.setPosition(stbl.payload);
    const auto end = stbl.payload + stbl.payloadSize;
    Box b;
    while (in.getPosition() < end && nextBox(in, end, b)) {
        const auto next = b.payload + b.payloadSize;
        const auto body = slurp(in, b);
        const auto* p = body.data();
        const auto n = body.size();
        if (b.type == fourcc("stsd") && n >= 16) {
            const auto* entry = p + 8;
            const std::uint32_t fmt = be32(entry + 4);
            if (fmt == fourcc("Hap1") || fmt == fourcc("Hap5")
                || fmt == fourcc("HapY") || fmt == fourcc("HapM")
                || fmt == fourcc("HapA")) {
                t.isHap = true;
                if (n >= 8 + 36) {
                    t.width = be16(entry + 32);
                    t.height = be16(entry + 34);
                }
            }
        } else if (b.type == fourcc("stts") && n >= 16) {
            t.delta = be32(p + 12);
        } else if (b.type == fourcc("stsz") && n >= 12) {
            const std::uint32_t uniform = be32(p + 4);
            const std::uint32_t count = be32(p + 8);
            t.sizes.assign(count, uniform);
            if (uniform == 0)
                for (std::uint32_t i = 0; i < count && 12 + 4 * (size_t) i + 4 <= n; ++i)
                    t.sizes[i] = be32(p + 12 + 4 * (size_t) i);
        } else if ((b.type == fourcc("stco") || b.type == fourcc("co64")) && n >= 8) {
            const std::uint32_t count = be32(p + 4);
            const bool wide = b.type == fourcc("co64");
            for (std::uint32_t i = 0; i < count; ++i) {
                const size_t at = 8 + (wide ? 8 : 4) * (size_t) i;
                if (at + (wide ? 8u : 4u) > n) break;
                t.chunkOffsets.push_back(
                    wide ? (std::int64_t) (((std::uint64_t) be32(p + at) << 32)
                                           | be32(p + at + 4))
                         : (std::int64_t) be32(p + at));
            }
        } else if (b.type == fourcc("stsc") && n >= 8) {
            const std::uint32_t count = be32(p + 4);
            for (std::uint32_t i = 0; i < count; ++i) {
                const size_t at = 8 + 12 * (size_t) i;
                if (at + 12 > n) break;
                t.runs.push_back({be32(p + at), be32(p + at + 4)});
            }
        }
        in.setPosition(next);
    }
}

inline void parseTrak(juce::InputStream& in, const Box& trak, TrackTables& t) {
    in.setPosition(trak.payload);
    const auto end = trak.payload + trak.payloadSize;
    Box b;
    while (in.getPosition() < end && nextBox(in, end, b)) {
        const auto next = b.payload + b.payloadSize;
        if (b.type == fourcc("mdia") || b.type == fourcc("minf")) {
            parseTrak(in, b, t);
        } else if (b.type == fourcc("mdhd")) {
            const auto body = slurp(in, b);
            if (body.size() >= 16) t.timescale = be32(body.data() + 12);
        } else if (b.type == fourcc("stbl")) {
            parseStbl(in, b, t);
        }
        in.setPosition(next);
    }
}

}

inline Movie open(const juce::File& file) {
    Movie m;
    juce::FileInputStream in(file);
    if (!in.openedOk()) return m;
    const auto end = in.getTotalLength();
    detail::Box b;
    detail::TrackTables best;
    while (in.getPosition() < end && detail::nextBox(in, end, b)) {
        const auto next = b.payload + b.payloadSize;
        if (b.type == detail::fourcc("moov")) {
            in.setPosition(b.payload);
            const auto moovEnd = b.payload + b.payloadSize;
            detail::Box inner;
            while (in.getPosition() < moovEnd && detail::nextBox(in, moovEnd, inner)) {
                const auto innerNext = inner.payload + inner.payloadSize;
                if (inner.type == detail::fourcc("trak")) {
                    detail::TrackTables t;
                    detail::parseTrak(in, inner, t);
                    if (t.isHap && !t.sizes.empty()) best = t;
                }
                in.setPosition(innerNext);
            }
        }
        in.setPosition(next);
    }
    if (!best.isHap || best.sizes.empty() || best.chunkOffsets.empty()) return m;

    size_t sample = 0;
    for (size_t chunk = 0; chunk < best.chunkOffsets.size() && sample < best.sizes.size();
         ++chunk) {
        std::uint32_t perChunk = 1;
        for (const auto& r : best.runs)
            if (r.firstChunk <= (std::uint32_t) chunk + 1) perChunk = r.perChunk;
        auto offset = best.chunkOffsets[chunk];
        for (std::uint32_t k = 0; k < perChunk && sample < best.sizes.size(); ++k) {
            m.samples.push_back({offset, best.sizes[sample]});
            offset += best.sizes[sample];
            ++sample;
        }
    }
    m.width = best.width;
    m.height = best.height;
    if (best.timescale > 0 && best.delta > 0)
        m.fps = (double) best.timescale / (double) best.delta;
    m.ok = m.width > 0 && m.height > 0 && !m.samples.empty();
    return m;
}

struct Frame {
    Tex tex = Tex::None;
    std::vector<std::uint8_t> blocks;
};

namespace detail {

struct Section {
    std::uint8_t type = 0;
    const std::uint8_t* data = nullptr;
    size_t size = 0, next = 0;
};

inline bool section(const std::uint8_t* p, size_t n, size_t at, Section& out) {
    if (at + 4 > n) return false;
    size_t size = (size_t) p[at] | ((size_t) p[at + 1] << 8) | ((size_t) p[at + 2] << 16);
    out.type = p[at + 3];
    size_t header = 4;
    if (size == 0) {
        if (at + 8 > n) return false;
        size = le32(p + at + 4);
        header = 8;
    }
    if (at + header + size > n) return false;
    out.data = p + at + header;
    out.size = size;
    out.next = at + header + size;
    return true;
}

inline Tex texOf(std::uint8_t type) {
    switch (type & 0x0F) {
        case 0x0B: return Tex::DXT1;
        case 0x0E: return Tex::DXT5;
        case 0x0F: return Tex::YCoCgDXT5;
        default: return Tex::None;
    }
}

}

inline bool decodeFrame(const std::uint8_t* p, size_t n, Frame& out) {
    using namespace detail;
    Section top;
    if (!section(p, n, 0, top)) return false;
    out.tex = texOf(top.type);
    if (out.tex == Tex::None) return false;
    const auto stage = top.type & 0xF0;
    if (stage == 0xA0) {
        out.blocks.assign(top.data, top.data + top.size);
        return true;
    }
    if (stage == 0xB0) return snappy::decode(top.data, top.size, out.blocks);
    if (stage != 0xC0) return false;

    Section instructions;
    if (!section(top.data, top.size, 0, instructions) || instructions.type != 0x01)
        return false;
    std::vector<std::uint8_t> compressors;
    std::vector<std::uint32_t> sizes;
    Section s;
    for (size_t at = 0; section(instructions.data, instructions.size, at, s);
         at = s.next) {
        if (s.type == 0x02) compressors.assign(s.data, s.data + s.size);
        if (s.type == 0x03)
            for (size_t k = 0; k + 4 <= s.size; k += 4)
                sizes.push_back(le32(s.data + k));
    }
    if (compressors.size() != sizes.size() || compressors.empty()) return false;
    const std::uint8_t* chunk = top.data + instructions.next;
    const auto* frameEnd = top.data + top.size;
    out.blocks.clear();
    std::vector<std::uint8_t> piece;
    for (size_t k = 0; k < sizes.size(); ++k) {
        if (chunk + sizes[k] > frameEnd) return false;
        if (compressors[k] == 0x0B) {
            if (!snappy::decode(chunk, sizes[k], piece)) return false;
            out.blocks.insert(out.blocks.end(), piece.begin(), piece.end());
        } else {
            out.blocks.insert(out.blocks.end(), chunk, chunk + sizes[k]);
        }
        chunk += sizes[k];
    }
    return true;
}

}
