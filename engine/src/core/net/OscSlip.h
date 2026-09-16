// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace hum::osc {

constexpr std::uint8_t kSlipEnd = 0xC0;
constexpr std::uint8_t kSlipEsc = 0xDB;
constexpr std::uint8_t kSlipEscEnd = 0xDC;
constexpr std::uint8_t kSlipEscEsc = 0xDD;

inline std::vector<std::uint8_t> slipEncode(const std::vector<std::uint8_t>& packet) {
    std::vector<std::uint8_t> out;
    out.reserve(packet.size() + 2);
    out.push_back(kSlipEnd);
    for (const auto b : packet) {
        if (b == kSlipEnd) { out.push_back(kSlipEsc); out.push_back(kSlipEscEnd); }
        else if (b == kSlipEsc) { out.push_back(kSlipEsc); out.push_back(kSlipEscEsc); }
        else out.push_back(b);
    }
    out.push_back(kSlipEnd);
    return out;
}

class SlipDecoder {
public:
    template <typename OnPacket>
    void feed(const std::uint8_t* bytes, int n, OnPacket&& onPacket) {
        for (int i = 0; i < n; ++i) {
            const std::uint8_t b = bytes[i];
            if (b == kSlipEnd) {
                if (!packet_.empty()) onPacket(packet_);
                packet_.clear();
                escaped_ = false;
                continue;
            }
            if (escaped_) {
                escaped_ = false;
                packet_.push_back(b == kSlipEscEnd ? kSlipEnd : b == kSlipEscEsc ? kSlipEsc : b);
                continue;
            }
            if (b == kSlipEsc) { escaped_ = true; continue; }
            packet_.push_back(b);
            if (packet_.size() > kMaxPacket) packet_.clear();
        }
    }

private:
    static constexpr size_t kMaxPacket = 4096;
    std::vector<std::uint8_t> packet_;
    bool escaped_ = false;
};

struct Arg {
    char tag = 'f';
    float f = 0.0f;
    std::int32_t i = 0;
    std::string s;
};

struct Message {
    std::string address;
    std::vector<Arg> args;
};

inline void putPadded(std::vector<std::uint8_t>& out, const std::string& s) {
    out.insert(out.end(), s.begin(), s.end());
    out.push_back(0);
    while (out.size() % 4 != 0) out.push_back(0);
}

inline void putU32(std::vector<std::uint8_t>& out, std::uint32_t v) {
    out.push_back((std::uint8_t) (v >> 24));
    out.push_back((std::uint8_t) (v >> 16));
    out.push_back((std::uint8_t) (v >> 8));
    out.push_back((std::uint8_t) v);
}

inline std::vector<std::uint8_t> encode(const Message& m) {
    std::vector<std::uint8_t> out;
    putPadded(out, m.address);
    std::string tags = ",";
    for (const auto& a : m.args) tags += a.tag;
    putPadded(out, tags);
    for (const auto& a : m.args) {
        if (a.tag == 'f') { std::uint32_t bits; std::memcpy(&bits, &a.f, 4); putU32(out, bits); }
        else if (a.tag == 'i') putU32(out, (std::uint32_t) a.i);
        else if (a.tag == 's') putPadded(out, a.s);
    }
    return out;
}

inline std::vector<std::uint8_t> encodeValue(const std::string& address, float value) {
    Message m;
    m.address = address;
    Arg a;
    a.f = value;
    m.args.push_back(a);
    return encode(m);
}

struct Reader {
    const std::uint8_t* data;
    size_t len;
    size_t pos = 0;

    bool string(std::string& out) {
        const size_t start = pos;
        while (pos < len && data[pos] != 0) ++pos;
        if (pos >= len) return false;
        out.assign((const char*) data + start, pos - start);
        ++pos;
        while (pos % 4 != 0) ++pos;
        return pos <= len;
    }
    bool u32(std::uint32_t& v) {
        if (pos + 4 > len) return false;
        v = ((std::uint32_t) data[pos] << 24) | ((std::uint32_t) data[pos + 1] << 16)
          | ((std::uint32_t) data[pos + 2] << 8) | (std::uint32_t) data[pos + 3];
        pos += 4;
        return true;
    }
};

inline bool decodeMessage(const std::uint8_t* data, size_t len, Message& m) {
    Reader r{data, len};
    if (!r.string(m.address) || m.address.empty() || m.address[0] != '/') return false;
    m.args.clear();
    if (r.pos >= len) return true;
    std::string tags;
    if (!r.string(tags) || tags.empty() || tags[0] != ',') return false;
    for (size_t k = 1; k < tags.size(); ++k) {
        Arg a;
        a.tag = tags[k];
        std::uint32_t bits = 0;
        switch (a.tag) {
            case 'f': if (!r.u32(bits)) return false; std::memcpy(&a.f, &bits, 4); break;
            case 'i': if (!r.u32(bits)) return false; a.i = (std::int32_t) bits; break;
            case 's': if (!r.string(a.s)) return false; break;
            case 'T': a.tag = 'i'; a.i = 1; break;
            case 'F': a.tag = 'i'; a.i = 0; break;
            case 'd': { std::uint32_t hi, lo; if (!r.u32(hi) || !r.u32(lo)) return false;
                        const std::uint64_t w = ((std::uint64_t) hi << 32) | lo; double d; std::memcpy(&d, &w, 8);
                        a.tag = 'f'; a.f = (float) d; break; }
            case 'b': { if (!r.u32(bits)) return false; r.pos += bits; while (r.pos % 4 != 0) ++r.pos;
                        if (r.pos > len) return false; continue; }
            default: continue;
        }
        m.args.push_back(a);
    }
    return true;
}

template <typename OnMessage>
inline void decodePacket(const std::uint8_t* data, size_t len, OnMessage&& onMessage) {
    if (len >= 8 && std::memcmp(data, "#bundle", 8) == 0) {
        Reader r{data, len, 16};
        while (r.pos + 4 <= len) {
            std::uint32_t size = 0;
            if (!r.u32(size) || r.pos + size > len) return;
            decodePacket(data + r.pos, size, onMessage);
            r.pos += size;
        }
        return;
    }
    Message m;
    if (decodeMessage(data, len, m)) onMessage(m);
}

}
