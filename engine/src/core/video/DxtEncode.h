// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "core/video/DxtDecode.h"

namespace hum::dxt {

namespace detail {

inline std::uint16_t to565(const int* rgb) {
    return (std::uint16_t) (((rgb[0] >> 3) << 11) | ((rgb[1] >> 2) << 5) | (rgb[2] >> 3));
}

struct Palette {
    std::uint8_t rgb[4][3];
};

inline Palette fourColours(std::uint16_t c0, std::uint16_t c1) {
    Palette p{};
    rgb565(c0, p.rgb[0]);
    rgb565(c1, p.rgb[1]);
    for (int ch = 0; ch < 3; ++ch) {
        p.rgb[2][ch] = (std::uint8_t) ((2 * p.rgb[0][ch] + p.rgb[1][ch] + 1) / 3);
        p.rgb[3][ch] = (std::uint8_t) ((p.rgb[0][ch] + 2 * p.rgb[1][ch] + 1) / 3);
    }
    return p;
}

inline int apart(const std::uint8_t* a, const std::uint8_t* b) {
    const int dr = a[0] - b[0], dg = a[1] - b[1], db = a[2] - b[2];
    return dr * dr + dg * dg + db * db;
}

inline void gather(const std::uint8_t* rgba, int width, int height, int bx, int by,
                   std::uint8_t px[16][3]) {
    for (int p = 0; p < 16; ++p) {
        const int x = std::min(bx * 4 + p % 4, width - 1);
        const int y = std::min(by * 4 + p / 4, height - 1);
        const std::uint8_t* src =
            rgba + ((std::size_t) y * (std::size_t) width + (std::size_t) x) * 4;
        px[p][0] = src[0];
        px[p][1] = src[1];
        px[p][2] = src[2];
    }
}

inline void endpoints(const std::uint8_t px[16][3], int lo[3], int hi[3]) {
    for (int ch = 0; ch < 3; ++ch) {
        lo[ch] = 255;
        hi[ch] = 0;
    }
    for (int p = 0; p < 16; ++p)
        for (int ch = 0; ch < 3; ++ch) {
            lo[ch] = std::min(lo[ch], (int) px[p][ch]);
            hi[ch] = std::max(hi[ch], (int) px[p][ch]);
        }
    for (int ch = 0; ch < 3; ++ch) {
        const int inset = (hi[ch] - lo[ch]) >> 4;
        lo[ch] = std::min(lo[ch] + inset, 255);
        hi[ch] = std::max(hi[ch] - inset, 0);
    }
}

inline void assign(const std::uint8_t px[16][3], std::uint16_t c0, std::uint16_t c1,
                   int idx[16]) {
    const auto pal = fourColours(c0, c1);
    for (int p = 0; p < 16; ++p) {
        int best = 0, closest = apart(px[p], pal.rgb[0]);
        for (int k = 1; k < 4; ++k)
            if (const int d = apart(px[p], pal.rgb[k]); d < closest) {
                closest = d;
                best = k;
            }
        idx[p] = best;
    }
}

inline bool refit(const std::uint8_t px[16][3], const int idx[16], std::uint16_t& c0,
                  std::uint16_t& c1) {
    static const double weight[4] = {0.0, 1.0, 1.0 / 3.0, 2.0 / 3.0};
    double aa = 0.0, ab = 0.0, bb = 0.0;
    double ax[3] = {0.0, 0.0, 0.0}, bx[3] = {0.0, 0.0, 0.0};
    for (int p = 0; p < 16; ++p) {
        const double b = weight[idx[p]], a = 1.0 - b;
        aa += a * a;
        ab += a * b;
        bb += b * b;
        for (int ch = 0; ch < 3; ++ch) {
            ax[ch] += a * px[p][ch];
            bx[ch] += b * px[p][ch];
        }
    }
    const double det = aa * bb - ab * ab;
    if (std::abs(det) < 1.0e-6) return false;
    int fit0[3], fit1[3];
    for (int ch = 0; ch < 3; ++ch) {
        fit0[ch] = (int) std::lround((bb * ax[ch] - ab * bx[ch]) / det);
        fit1[ch] = (int) std::lround((aa * bx[ch] - ab * ax[ch]) / det);
        fit0[ch] = std::clamp(fit0[ch], 0, 255);
        fit1[ch] = std::clamp(fit1[ch], 0, 255);
    }
    c0 = to565(fit0);
    c1 = to565(fit1);
    if (c0 < c1) std::swap(c0, c1);
    return true;
}

inline void colourBlock(const std::uint8_t px[16][3], std::uint8_t* out) {
    int lo[3], hi[3];
    endpoints(px, lo, hi);
    std::uint16_t c0 = to565(hi), c1 = to565(lo);
    if (c0 < c1) std::swap(c0, c1);
    int idx[16] = {};
    if (c0 != c1) {
        assign(px, c0, c1, idx);
        if (refit(px, idx, c0, c1) && c0 != c1) assign(px, c0, c1, idx);
    }
    out[0] = (std::uint8_t) (c0 & 0xFF);
    out[1] = (std::uint8_t) (c0 >> 8);
    out[2] = (std::uint8_t) (c1 & 0xFF);
    out[3] = (std::uint8_t) (c1 >> 8);
    out[4] = out[5] = out[6] = out[7] = 0;
    if (c0 == c1) return;
    for (int p = 0; p < 16; ++p)
        out[4 + p / 4] = (std::uint8_t) (out[4 + p / 4] | (idx[p] << ((p % 4) * 2)));
}

}

inline void encodeDxt1(const std::uint8_t* rgba, int width, int height,
                       std::vector<std::uint8_t>& out) {
    if (width <= 0 || height <= 0) {
        out.clear();
        return;
    }
    const int bw = (width + 3) / 4, bh = (height + 3) / 4;
    out.assign(encodedSize(Kind::DXT1, width, height), 0);
    std::uint8_t px[16][3];
    for (int by = 0; by < bh; ++by)
        for (int bx = 0; bx < bw; ++bx) {
            detail::gather(rgba, width, height, bx, by, px);
            detail::colourBlock(px, out.data()
                                        + ((std::size_t) by * (std::size_t) bw
                                           + (std::size_t) bx) * blockBytes(Kind::DXT1));
        }
}

}
