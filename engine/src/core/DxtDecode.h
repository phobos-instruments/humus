#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace hum::dxt {

enum class Kind { DXT1, DXT5, YCoCgDXT5 };

inline std::size_t blockBytes(Kind k) { return k == Kind::DXT1 ? 8 : 16; }

inline std::size_t encodedSize(Kind k, int width, int height) {
    const std::size_t bw = (std::size_t) ((width + 3) / 4), bh = (std::size_t) ((height + 3) / 4);
    return bw * bh * blockBytes(k);
}

inline void rgb565(std::uint16_t c, std::uint8_t* rgb) {
    const int r = (c >> 11) & 31, g = (c >> 5) & 63, b = c & 31;
    rgb[0] = (std::uint8_t) ((r * 255 + 15) / 31);
    rgb[1] = (std::uint8_t) ((g * 255 + 31) / 63);
    rgb[2] = (std::uint8_t) ((b * 255 + 15) / 31);
}

struct Block {
    std::uint8_t rgba[16][4];
};

inline void decodeColourBlock(const std::uint8_t* in, bool fourColoursAlways, Block& out) {
    const std::uint16_t c0 = (std::uint16_t) (in[0] | (in[1] << 8));
    const std::uint16_t c1 = (std::uint16_t) (in[2] | (in[3] << 8));
    std::uint8_t pal[4][4];
    rgb565(c0, pal[0]);
    rgb565(c1, pal[1]);
    pal[0][3] = pal[1][3] = 255;
    const bool four = fourColoursAlways || c0 > c1;
    for (int ch = 0; ch < 3; ++ch) {
        if (four) {
            pal[2][ch] = (std::uint8_t) ((2 * pal[0][ch] + pal[1][ch] + 1) / 3);
            pal[3][ch] = (std::uint8_t) ((pal[0][ch] + 2 * pal[1][ch] + 1) / 3);
        } else {
            pal[2][ch] = (std::uint8_t) ((pal[0][ch] + pal[1][ch] + 1) / 2);
            pal[3][ch] = 0;
        }
    }
    pal[2][3] = 255;
    pal[3][3] = four ? 255 : 0;
    for (int p = 0; p < 16; ++p) {
        const int idx = (in[4 + p / 4] >> ((p % 4) * 2)) & 3;
        for (int ch = 0; ch < 4; ++ch) out.rgba[p][ch] = pal[idx][ch];
    }
}

inline void decodeAlphaBlock(const std::uint8_t* in, Block& out) {
    const int a0 = in[0], a1 = in[1];
    std::uint8_t pal[8];
    pal[0] = (std::uint8_t) a0;
    pal[1] = (std::uint8_t) a1;
    if (a0 > a1) {
        for (int i = 1; i <= 6; ++i) pal[i + 1] = (std::uint8_t) (((7 - i) * a0 + i * a1) / 7);
    } else {
        for (int i = 1; i <= 4; ++i) pal[i + 1] = (std::uint8_t) (((5 - i) * a0 + i * a1) / 5);
        pal[6] = 0;
        pal[7] = 255;
    }
    std::uint64_t bits = 0;
    for (int i = 0; i < 6; ++i) bits |= (std::uint64_t) in[2 + i] << (8 * i);
    for (int p = 0; p < 16; ++p) out.rgba[p][3] = pal[(bits >> (3 * p)) & 7];
}

inline void yCoCgToRgb(std::uint8_t* px) {
    const float scale = (float) px[2] / 8.0f + 1.0f;
    const float co = ((float) px[0] / 255.0f - 0.50196078f) / scale;
    const float cg = ((float) px[1] / 255.0f - 0.50196078f) / scale;
    const float y = (float) px[3] / 255.0f;
    const float rgb[3] = {y + co - cg, y + cg, y - co - cg};
    for (int ch = 0; ch < 3; ++ch)
        px[ch] = (std::uint8_t) std::clamp((int) (rgb[ch] * 255.0f + 0.5f), 0, 255);
    px[3] = 255;
}

inline bool decodeToBgra(Kind kind, const std::uint8_t* blocks, std::size_t size, int width,
                         int height, std::vector<std::uint8_t>& bgra) {
    if (width <= 0 || height <= 0 || size < encodedSize(kind, width, height)) return false;
    bgra.assign((std::size_t) width * (std::size_t) height * 4, 0);
    const int bw = (width + 3) / 4, bh = (height + 3) / 4;
    const std::size_t stride = blockBytes(kind);
    Block blk;
    for (int by = 0; by < bh; ++by) {
        for (int bx = 0; bx < bw; ++bx) {
            const std::uint8_t* in = blocks + ((std::size_t) by * (std::size_t) bw + (std::size_t) bx) * stride;
            if (kind == Kind::DXT1) {
                decodeColourBlock(in, false, blk);
            } else {
                decodeAlphaBlock(in, blk);
                Block colour;
                decodeColourBlock(in + 8, true, colour);
                for (int p = 0; p < 16; ++p)
                    for (int ch = 0; ch < 3; ++ch) blk.rgba[p][ch] = colour.rgba[p][ch];
                if (kind == Kind::YCoCgDXT5)
                    for (int p = 0; p < 16; ++p) yCoCgToRgb(blk.rgba[p]);
            }
            for (int p = 0; p < 16; ++p) {
                const int x = bx * 4 + p % 4, y = by * 4 + p / 4;
                if (x >= width || y >= height) continue;
                std::uint8_t* out = bgra.data() + ((std::size_t) y * (std::size_t) width + (std::size_t) x) * 4;
                out[0] = blk.rgba[p][2];
                out[1] = blk.rgba[p][1];
                out[2] = blk.rgba[p][0];
                out[3] = blk.rgba[p][3];
            }
        }
    }
    return true;
}

}
