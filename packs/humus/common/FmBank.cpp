// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "common/FmBank.h"

#include <cstring>

#include "hum/dsp/DspMath.h"

namespace hum::fmbank {

bool wopnLayout(const uint8_t* d, size_t n, WopnLayout& out) {
    if (d == nullptr || n < 20) return false;
    const bool magic2 = std::memcmp(d, "WOPN2-B2NK\0", 11) == 0;
    if (!magic2 && std::memcmp(d, "WOPN2-BANK\0", 11) != 0) return false;
    size_t off = 11;
    int version = 1;
    if (magic2) { version = d[off] | (d[off + 1] << 8); off += 2; }
    out.melodicBanks = (d[off] << 8) | d[off + 1];
    out.percussionBanks = (d[off + 2] << 8) | d[off + 3];
    off += 4 + 1;
    const int banks = out.melodicBanks + out.percussionBanks;
    if (banks <= 0 || banks > 128) return false;
    if (version >= 2) off += (size_t) banks * 34;
    out.stride = version >= 2 ? 69 : 65;
    out.first = off;
    return off + (size_t) banks * 128 * out.stride <= n;
}

std::string wopnName(const uint8_t* e) {
    std::string s;
    for (int i = 0; i < 32 && e[i] != 0; ++i)
        s += (e[i] < 32 || e[i] > 126) ? '?' : (char) e[i];
    while (!s.empty() && s.back() == ' ') s.pop_back();
    size_t b = 0;
    while (b < s.size() && s[b] == ' ') ++b;
    return s.substr(b);
}

FmChip::Patch wopnPatch(const uint8_t* e) {
    FmChip::Patch p;
    p.alg = (uint8_t) (e[35] & 7);
    p.fb = (uint8_t) ((e[35] >> 3) & 7);
    for (int o = 0; o < 4; ++o) {
        const uint8_t* ob = e + 37 + o * 7;
        FmChip::Op& op = p.ops[(size_t) o];
        const int regDt = (ob[0] >> 4) & 7;
        op.dt = (uint8_t) (regDt <= 3 ? 3 + regDt : 3 - (regDt - 4));
        op.mult = (uint8_t) (ob[0] & 15);
        op.tl = (uint8_t) (ob[1] & kSevenBitMax);
        op.rs = (uint8_t) ((ob[2] >> 6) & 3);
        op.ar = (uint8_t) (ob[2] & 31);
        op.dr = (uint8_t) (ob[3] & 31);
        op.d2r = (uint8_t) (ob[4] & 31);
        op.sl = (uint8_t) ((ob[5] >> 4) & 15);
        op.rr = (uint8_t) (ob[5] & 15);
        op.ssg = ob[6];
    }
    return p;
}

}
