#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace hum::snappy {

inline bool readVarint(const std::uint8_t* p, size_t n, size_t& at, std::uint32_t& out) {
    out = 0;
    for (int shift = 0; shift < 35 && at < n; shift += 7) {
        const std::uint8_t b = p[at++];
        out |= (std::uint32_t) (b & 0x7F) << shift;
        if ((b & 0x80) == 0) return true;
    }
    return false;
}

inline bool decode(const std::uint8_t* p, size_t n, std::vector<std::uint8_t>& out) {
    size_t at = 0;
    std::uint32_t total = 0;
    if (!readVarint(p, n, at, total)) return false;
    out.resize(total);
    size_t w = 0;
    while (at < n) {
        const std::uint8_t tag = p[at++];
        const int kind = tag & 3;
        if (kind == 0) {
            size_t len = (size_t) (tag >> 2) + 1;
            if (len > 60) {
                const int extra = (int) len - 60;
                if (at + (size_t) extra > n) return false;
                len = 0;
                for (int i = 0; i < extra; ++i) len |= (size_t) p[at + (size_t) i] << (8 * i);
                len += 1;
                at += (size_t) extra;
            }
            if (at + len > n || w + len > total) return false;
            std::memcpy(out.data() + w, p + at, len);
            at += len;
            w += len;
            continue;
        }
        size_t len = 0, offset = 0;
        if (kind == 1) {
            if (at + 1 > n) return false;
            len = ((size_t) (tag >> 2) & 0x7) + 4;
            offset = ((size_t) (tag >> 5) << 8) | p[at];
            at += 1;
        } else if (kind == 2) {
            if (at + 2 > n) return false;
            len = (size_t) (tag >> 2) + 1;
            offset = (size_t) p[at] | ((size_t) p[at + 1] << 8);
            at += 2;
        } else {
            if (at + 4 > n) return false;
            len = (size_t) (tag >> 2) + 1;
            offset = (size_t) p[at] | ((size_t) p[at + 1] << 8)
                   | ((size_t) p[at + 2] << 16) | ((size_t) p[at + 3] << 24);
            at += 4;
        }
        if (offset == 0 || offset > w || w + len > total) return false;
        for (size_t i = 0; i < len; ++i, ++w) out[w] = out[w - offset];
    }
    return w == total;
}

}
