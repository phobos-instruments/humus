// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <set>
#include <string>
#include <vector>

#include "hum/FileBytes.h"

#include "hum/dsp/DspMath.h"
#include "hum/dsp/NsmpCodec.h"

namespace hum::nsmp {

struct Stroke {
    int globalId = 0;
    int rootKey = 60;
    int channels = 1;
    double sampleRate = kPitchBase;
    std::vector<std::vector<std::int32_t>> pcm;
    int loopStart = 0, loopEnd = 0;
    bool loops = false;
};

struct Zone {
    int stroke = -1;
    int rootKey = 60;
    int keyLo = 0, keyHi = kMidiMax, velLo = 0, velHi = kMidiMax;
};

struct NsmpData {
    bool parsed = false;
    int codec = 0;
    std::string name;
    bool factory = false;
    std::vector<Stroke> strokes;
    std::vector<Zone> zones;
};

inline bool isNsmpPath(const std::string& uri) {
    const auto ext = lowerExtension(uri);
    return ext == ".nsmp3" || ext == ".nsmp4";
}

namespace detail {

struct Reader {
    const std::uint8_t* d; std::size_t n;
    std::uint8_t u8(std::size_t o) const { return o < n ? d[o] : 0; }
    std::uint16_t u16(std::size_t o) const { return (std::uint16_t) ((u8(o) << 8) | u8(o + 1)); }
    std::uint32_t u32(std::size_t o) const { return o + 4 <= n ? readWord(d, o) : 0u; }
};

struct Section {
    std::string tag;
    std::uint32_t version = 0;
    std::size_t off = 0, end = 0;
};

inline std::vector<Section> sections(const Reader& r, std::size_t body) {
    std::vector<Section> out;
    for (std::size_t o = body; o + 12 <= r.n;) {
        Section s;
        for (int i = 0; i < 4; ++i)
            if (const char c = (char) r.u8(o + (std::size_t) i); c != 0) s.tag.push_back(c);
        s.version = r.u32(o + 4);
        const std::size_t size = r.u32(o + 8);
        s.off = o + 12;
        s.end = s.off + size;
        if (s.end > r.n) break;
        out.push_back(s);
        o = s.end;
    }
    return out;
}

inline std::string printableRun(const Reader& r, std::size_t from, std::size_t to) {
    std::string run;
    for (std::size_t i = from; i < to && i < r.n; ++i) {
        const auto c = r.u8(i);
        if (c >= 0x20 && c < 0x7f) run.push_back((char) c);
        else if (run.size() >= 2) break;
        else run.clear();
    }
    while (!run.empty() && run.back() == ' ') run.pop_back();
    return run;
}

inline std::size_t streamStart(const Reader& r, const Section& s, int channels, bool codec4,
                               Decoded& best) {
    for (std::size_t hdr = 0x60; hdr <= 0x180; hdr += 4) {
        const std::size_t start = s.off + hdr;
        if (start >= s.end) break;
        auto got = decodeStream(r.d, start, s.end, channels, codec4);
        if (got.ok && got.end + 8 >= s.end) { best = std::move(got); return start; }
    }
    return 0;
}

inline void applyLoop(const Reader& r, const Section& s, const Decoded& dec, Stroke& st) {
    const std::uint32_t u1 = r.u32(s.off + 0x12), u2 = r.u32(s.off + 0x1b);
    const std::uint32_t u3 = r.u32(s.off + 0x24), u4 = r.u32(s.off + 0x2d);
    if (!(u1 <= u2 && u2 <= u3 && u3 < u4)) return;
    auto at = [&](std::uint32_t rel) -> int {
        for (const auto& m : dec.marks)
            if (m.word == dec.firstDataWord + rel) return (int) (m.position / (std::size_t) st.channels);
        return -1;
    };
    const int in = at(u2 - u1), out = at(u3 - u1);
    if (in < 0 || out <= in) return;
    st.loopStart = in;
    st.loopEnd = out;
    st.loops = true;
}

inline bool readStroke(const Reader& r, const Section& s, bool codec4, Stroke& st) {
    st.globalId = r.u8(s.off + 3);
    st.rootKey = std::min<int>(r.u8(s.off + 5), kMidiMax);
    const int rate = r.u16(s.off + 6);
    st.sampleRate = rate >= 8000 && rate <= 96000 ? rate : kPitchBase;
    const int hint = r.u8(s.off + 8);
    const std::vector<int> order = hint == 1 ? std::vector<int>{1, 2}
                                 : hint == 2 ? std::vector<int>{2, 1} : std::vector<int>{1, 2};
    for (const int channels : order) {
        Decoded dec;
        if (streamStart(r, s, channels, codec4, dec) == 0) continue;
        st.channels = channels;
        st.pcm = std::move(dec.pcm);
        applyLoop(r, s, dec, st);
        return true;
    }
    return false;
}

struct StrokeIndex {
    std::vector<std::vector<int>> byGid = std::vector<std::vector<int>>(256);
    const std::vector<Stroke>* strokes = nullptr;

    bool known(int gid) const {
        return gid >= 0 && gid < (int) byGid.size() && !byGid[(std::size_t) gid].empty();
    }

    int resolve(int gid, int rootKey) const {
        if (!known(gid)) return -1;
        const auto& same = byGid[(std::size_t) gid];
        if (same.size() > 1 && strokes != nullptr)
            for (const int i : same)
                if ((*strokes)[(std::size_t) i].rootKey == rootKey) return i;
        return same.back();
    }
};

inline Zone recordZone(const Reader& r, std::size_t o, const StrokeIndex& index) {
    Zone z;
    z.rootKey = r.u8(o);
    z.keyHi = r.u8(o + 1);
    z.keyLo = r.u8(o + 2);
    z.stroke = index.resolve(r.u8(o + 11), z.rootKey);
    z.velLo = r.u8(o + 14);
    z.velHi = r.u8(o + 15);
    return z;
}

inline bool recordLooksReal(const Reader& r, std::size_t o, std::size_t end,
                            const StrokeIndex& index) {
    if (o + 16 > end) return false;
    const bool keys = r.u8(o) <= kMidiMax && r.u8(o + 1) <= kMidiMax && r.u8(o + 2) <= kMidiMax;
    const bool marker = (r.u8(o + 6) == 0 && r.u8(o + 7) == 1) || (r.u8(o + 12) == 0 && r.u8(o + 13) == 1);
    return keys && marker && index.known(r.u8(o + 11));
}

inline std::vector<Zone> fixedRecords(const Reader& r, const Section& map, std::size_t recStart,
                                      const StrokeIndex& index) {
    std::vector<Zone> zones;
    const int count = r.u8(recStart - 1);
    if (count < 1 || recStart + (std::size_t) count * 16 > map.end + 0) return zones;
    if (map.end < recStart + (std::size_t) count * 16 || map.end - (recStart + (std::size_t) count * 16) > 8) return zones;
    if (!recordLooksReal(r, recStart, map.end, index)) return zones;
    for (int i = 0; i < count; ++i) zones.push_back(recordZone(r, recStart + (std::size_t) i * 16, index));
    return zones;
}

inline std::vector<Zone> scannedRecords(const Reader& r, const Section& map,
                                        const StrokeIndex& index) {
    std::size_t bestStart = 0, bestN = 0, bestTrailer = SIZE_MAX;
    for (std::size_t p = map.off + 6; p + 16 <= map.end; ++p) {
        std::size_t n = 0;
        for (std::size_t o = p; recordLooksReal(r, o, map.end, index); o += 16) ++n;
        if (n == 0) continue;
        const std::size_t trailer = map.end - (p + n * 16);
        if (n > bestN || (n == bestN && trailer < bestTrailer)) { bestStart = p; bestN = n; bestTrailer = trailer; }
    }
    std::vector<Zone> zones;
    for (std::size_t i = 0; i < bestN; ++i) zones.push_back(recordZone(r, bestStart + i * 16, index));
    return zones;
}

inline std::vector<Zone> strokeRefRecords(const Reader& r, const Section& map,
                                          const StrokeIndex& index,
                                          const std::vector<Stroke>& strokes) {
    auto ok = [&](std::size_t o) {
        return o + 11 <= map.end && r.u8(o) == 0 && r.u8(o + 6) == 1 && r.u8(o + 3) >= 1
               && r.u8(o + 3) <= kMidiMax && index.known(r.u8(o + 10));
    };
    std::size_t bestStart = 0, bestN = 0, bestTrailer = SIZE_MAX;
    for (std::size_t p = map.off + 6; p + 11 <= map.end; ++p) {
        std::size_t n = 0;
        for (std::size_t o = p; ok(o); o += 11) ++n;
        if (n == 0) continue;
        const std::size_t trailer = map.end - (p + n * 11);
        if (n > bestN || (n == bestN && trailer < bestTrailer)) { bestStart = p; bestN = n; bestTrailer = trailer; }
    }
    std::vector<Zone> zones;
    for (std::size_t i = 0; i < bestN; ++i) {
        const std::size_t o = bestStart + i * 11;
        Zone z;
        z.keyHi = r.u8(o + 3);
        z.stroke = index.resolve(r.u8(o + 10), -1);
        z.rootKey = z.stroke >= 0 ? strokes[(std::size_t) z.stroke].rootKey : z.keyHi;
        zones.push_back(z);
    }
    std::sort(zones.begin(), zones.end(), [](const Zone& a, const Zone& b) { return a.keyHi > b.keyHi; });
    for (std::size_t i = 0; i < zones.size(); ++i)
        zones[i].keyLo = i + 1 < zones.size() ? zones[i + 1].keyHi + 1 : 0;
    return zones;
}

inline std::vector<Zone> readZones(const Reader& r, const Section& map, const std::vector<Stroke>& strokes) {
    StrokeIndex index;
    index.strokes = &strokes;
    for (std::size_t i = 0; i < strokes.size(); ++i)
        index.byGid[(std::size_t) (strokes[i].globalId & 0xff)].push_back((int) i);
    if (map.version >= 21) {
        auto zones = fixedRecords(r, map, map.off + 6 + 128 * 10 + 32, index);
        return zones.empty() ? scannedRecords(r, map, index) : zones;
    }
    if (map.version == 12) return strokeRefRecords(r, map, index, strokes);
    auto zones = fixedRecords(r, map, map.off + 6 + 128 * 6 + 1, index);
    return zones.empty() ? scannedRecords(r, map, index) : zones;
}

}

inline NsmpData load(const std::uint8_t* data, std::size_t size) {
    using namespace detail;
    NsmpData out;
    Reader r{data, size};
    if (size < 0x30 || std::string((const char*) data, 4) != "CBIN"
        || std::string((const char*) data + 8, 4) != "nsmp") return out;
    const int versionRaw = data[0x14] | (data[0x15] << 8);
    out.codec = versionRaw / 100;
    if (out.codec != 3 && out.codec != 4) return out;
    const std::size_t body = data[4] == 0 ? 0x18 : 0x2c;
    const auto secs = sections(r, body);
    const Section* map = nullptr;
    for (const auto& s : secs) {
        if (s.tag == "hdr") {
            out.factory = (r.u8(s.off + 8) | r.u8(s.off + 9)) != 0;
            out.name = printableRun(r, s.off + 8, s.end);
        } else if (s.tag == "map") {
            map = &s;
        } else if (s.tag == "stk") {
            Stroke st;
            if (readStroke(r, s, out.codec == 4, st)) out.strokes.push_back(std::move(st));
        }
    }
    if (map != nullptr) out.zones = readZones(r, *map, out.strokes);
    out.zones.erase(std::remove_if(out.zones.begin(), out.zones.end(),
                                   [](const Zone& z) { return z.stroke < 0 || z.keyLo > z.keyHi; }),
                    out.zones.end());
    out.parsed = !out.strokes.empty() && !out.zones.empty();
    return out;
}

inline NsmpData loadFile(const std::string& path) {
    std::vector<std::uint8_t> bytes;
    if (!readFileBytes(path, bytes) || bytes.size() < 0x30) return {};
    return load(bytes.data(), bytes.size());
}

}
