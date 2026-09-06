#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace hum::snappy {

namespace detail {

constexpr int kHashBits = 14;
constexpr std::size_t kMaxCopy = 64;
constexpr std::size_t kMinMatch = 4;

inline std::uint32_t load32(const std::uint8_t* p) {
    std::uint32_t v = 0;
    std::memcpy(&v, p, 4);
    return v;
}

inline std::uint32_t hash32(std::uint32_t v) { return (v * 0x1e35a7bdu) >> (32 - kHashBits); }

inline void putVarint(std::vector<std::uint8_t>& out, std::uint32_t v) {
    while (v >= 0x80) {
        out.push_back((std::uint8_t) ((v & 0x7F) | 0x80));
        v >>= 7;
    }
    out.push_back((std::uint8_t) v);
}

inline void putLiteral(std::vector<std::uint8_t>& out, const std::uint8_t* p, std::size_t n) {
    if (n == 0) return;
    const std::size_t len = n - 1;
    if (len < 60) {
        out.push_back((std::uint8_t) (len << 2));
    } else {
        int bytes = 1;
        for (std::size_t v = len >> 8; v > 0; v >>= 8) ++bytes;
        out.push_back((std::uint8_t) ((59 + bytes) << 2));
        for (int i = 0; i < bytes; ++i) out.push_back((std::uint8_t) ((len >> (8 * i)) & 0xFF));
    }
    out.insert(out.end(), p, p + n);
}

inline void putCopy(std::vector<std::uint8_t>& out, std::size_t offset, std::size_t len) {
    while (len > 0) {
        const std::size_t take = len > kMaxCopy ? kMaxCopy : len;
        if (take >= kMinMatch && take <= 11 && offset < 2048) {
            out.push_back((std::uint8_t) (1 | ((take - 4) << 2) | ((offset >> 8) << 5)));
            out.push_back((std::uint8_t) (offset & 0xFF));
        } else {
            out.push_back((std::uint8_t) (2 | ((take - 1) << 2)));
            out.push_back((std::uint8_t) (offset & 0xFF));
            out.push_back((std::uint8_t) ((offset >> 8) & 0xFF));
        }
        len -= take;
    }
}

}

inline bool encode(const std::uint8_t* p, std::size_t n, std::vector<std::uint8_t>& out) {
    using namespace detail;
    if (n > 0xFFFFFFFFull) return false;
    out.clear();
    out.reserve(n / 2 + 32);
    putVarint(out, (std::uint32_t) n);
    if (n < kMinMatch + 1) {
        putLiteral(out, p, n);
        return true;
    }
    std::vector<std::uint32_t> table((std::size_t) 1 << kHashBits, 0);
    std::vector<bool> seen((std::size_t) 1 << kHashBits, false);
    std::size_t at = 0, emitted = 0;
    while (at + kMinMatch <= n) {
        const std::uint32_t h = hash32(load32(p + at));
        const std::size_t candidate = table[h];
        const bool hit = seen[h] && at - candidate < 65536
                         && load32(p + candidate) == load32(p + at);
        table[h] = (std::uint32_t) at;
        seen[h] = true;
        if (!hit) {
            ++at;
            continue;
        }
        std::size_t len = kMinMatch;
        while (at + len < n && p[candidate + len] == p[at + len]) ++len;
        putLiteral(out, p + emitted, at - emitted);
        putCopy(out, at - candidate, len);
        at += len;
        emitted = at;
    }
    putLiteral(out, p + emitted, n - emitted);
    return true;
}

}
