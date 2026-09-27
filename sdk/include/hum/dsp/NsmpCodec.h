// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <utility>
#include <vector>

namespace hum::nsmp {

inline constexpr int kPitchBase = 0x88ba;
inline constexpr int kMaxOrder = 7;
inline constexpr std::int32_t kSaneMagnitude = 1 << 26;

inline constexpr std::array<std::array<std::int32_t, 7>, 8> kPredictor = {{
    {0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0},
    {2, -1, 0, 0, 0, 0, 0},
    {3, -3, 1, 0, 0, 0, 0},
    {4, -6, 4, -1, 0, 0, 0},
    {5, -10, 10, -5, 1, 0, 0},
    {6, -15, 20, -15, 6, -1, 0},
    {7, -21, 35, -35, 21, -7, 1},
}};

struct BlockHeader {
    int sampleCount = 0;
    int order = 0;
    int bitWidth = 1;
    bool linear = false;
    bool stop() const { return order == 0 && bitWidth == 1; }
};

inline std::uint32_t packBlockHeader(int sampleCount, int order, int bitWidth, bool linear) {
    return ((std::uint32_t) (linear ? 1 : 0) << 23) | ((std::uint32_t) (bitWidth - 1) << 19)
           | ((std::uint32_t) order << 14) | ((std::uint32_t) sampleCount & 0x3fff);
}

inline BlockHeader unpackBlockHeader(std::uint32_t word) {
    BlockHeader h;
    h.sampleCount = (int) (word & 0x3fff);
    h.order = (int) ((word >> 14) & 0xf);
    h.bitWidth = (int) ((word >> 19) & 0xf) + 1;
    h.linear = ((word >> 23) & 1) != 0;
    return h;
}

struct Predictor {
    std::array<std::int32_t, 32> ring{};
    int head = 0;
    std::int32_t emit(std::int32_t residual, int order) {
        std::int32_t pred = 0;
        for (int k = 0; k < order; ++k)
            pred += kPredictor[(std::size_t) order][(std::size_t) k] * ring[(std::size_t) ((head - 1 - k) & 31)];
        const std::int32_t sample = residual + pred;
        ring[(std::size_t) head] = sample;
        head = (head + 1) & 31;
        return sample;
    }
};

struct BitReader {
    std::uint64_t acc = 0;
    int bits = 0;
    void feed(std::uint32_t word) {
        acc |= (std::uint64_t) word << (32 - bits);
        bits += 32;
    }
    bool has(int width) const { return bits >= width; }
    std::int32_t take(int width) {
        const std::uint32_t raw = (std::uint32_t) ((acc >> (64 - width)) & ((1ull << width) - 1));
        acc <<= width;
        bits -= width;
        return raw >= (1u << (width - 1)) ? (std::int32_t) raw - (std::int32_t) (1u << width)
                                          : (std::int32_t) raw;
    }
};

struct Mark {
    std::size_t word = 0;
    std::size_t position = 0;
};

struct Decoded {
    bool ok = false;
    std::vector<std::vector<std::int32_t>> pcm;
    std::size_t end = 0;
    std::size_t firstDataWord = 0;
    std::vector<Mark> marks;
    std::int32_t peak = 0;
};

inline std::uint32_t readWord(const std::uint8_t* d, std::size_t o) {
    return ((std::uint32_t) d[o] << 24) | ((std::uint32_t) d[o + 1] << 16)
           | ((std::uint32_t) d[o + 2] << 8) | (std::uint32_t) d[o + 3];
}

inline Decoded decodeStream(const std::uint8_t* d, std::size_t start, std::size_t end,
                            int channels, bool wordInterleaved) {
    Decoded out;
    if (channels < 1 || channels > 2 || start + 4 > end) return out;
    out.pcm.assign((std::size_t) channels, {});
    std::vector<Predictor> state((std::size_t) channels);
    std::size_t o = start, position = 0;
    bool sawData = false;
    auto note = [&](std::int32_t v) {
        const std::int32_t a = std::abs(v);
        if (a > out.peak) out.peak = a;
    };
    while (o + 4 <= end) {
        const auto h = unpackBlockHeader(readWord(d, o));
        const std::size_t headerWord = (o - start) / 4;
        o += 4;
        if (wordInterleaved) {
            if (h.sampleCount == 0) continue;
            if (h.order > kMaxOrder) break;
            if (!sawData) { sawData = true; out.firstDataWord = headerWord; }
            if (h.linear && position > 0) out.marks.push_back({headerWord, position});
            const int perChannel = h.sampleCount / channels;
            const std::size_t wordsPerChannel = ((std::size_t) perChannel * (std::size_t) h.bitWidth + 31) / 32;
            if (o + wordsPerChannel * (std::size_t) channels * 4 > end) break;
            for (int ch = 0; ch < channels; ++ch) {
                const int count = ch == channels - 1 ? h.sampleCount - perChannel * (channels - 1) : perChannel;
                BitReader bits;
                int produced = 0;
                for (std::size_t w = 0; w < wordsPerChannel && produced < count; ++w) {
                    bits.feed(readWord(d, o + (w * (std::size_t) channels + (std::size_t) ch) * 4));
                    while (bits.has(h.bitWidth) && produced < count) {
                        const auto v = state[(std::size_t) ch].emit(bits.take(h.bitWidth), h.order);
                        out.pcm[(std::size_t) ch].push_back(v);
                        note(v);
                        ++produced;
                    }
                }
            }
            position += (std::size_t) h.sampleCount;
            o += wordsPerChannel * (std::size_t) channels * 4;
            continue;
        }
        if (h.stop()) break;
        if (h.order > kMaxOrder || h.sampleCount == 0) return out;
        if (!sawData) { sawData = true; out.firstDataWord = headerWord; }
        if (h.linear && position > 0) out.marks.push_back({headerWord, position});
        const std::size_t words = ((std::size_t) h.sampleCount * (std::size_t) h.bitWidth + 31) / 32;
        if (o + words * 4 > end) return out;
        BitReader bits;
        int produced = 0;
        for (std::size_t w = 0; w < words && produced < h.sampleCount; ++w) {
            bits.feed(readWord(d, o + w * 4));
            while (bits.has(h.bitWidth) && produced < h.sampleCount) {
                const std::size_t ch = position % (std::size_t) channels;
                const auto v = state[ch].emit(bits.take(h.bitWidth), h.order);
                out.pcm[ch].push_back(v);
                note(v);
                ++produced;
                ++position;
            }
        }
        o += words * 4;
    }
    out.end = o;
    out.ok = sawData && out.peak < kSaneMagnitude && out.pcm[0].size() > 100;
    return out;
}

}
