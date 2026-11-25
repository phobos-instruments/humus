// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

namespace hum::blocklane {

inline constexpr double kOpenEnd = -1.0;

struct Block {
    double start = 0.0;
    double end = kOpenEnd;
    int value = 0;
    int point = 0;
    bool open() const { return end < 0.0; }
};

template <typename P>
int roundedValue(const P& p) {
    return (int) std::lround(p.value);
}

template <typename P>
P held(double beat, int value) {
    P p;
    p.beat = beat;
    p.value = (double) value;
    p.valueMax = (double) value;
    p.release = false;
    return p;
}

template <typename P>
P gap(double beat, int faceplate) {
    P p = held<P>(beat, faceplate);
    p.release = true;
    return p;
}

template <typename P>
P stateAt(const std::vector<P>& pts, double beat, int faceplate) {
    if (pts.empty()) return gap<P>(beat, faceplate);
    P at = pts.front();
    for (const auto& p : pts) {
        if (p.beat > beat) break;
        at = p;
    }
    at.beat = beat;
    return at;
}

template <typename P>
std::vector<P> tidy(const std::vector<P>& pts) {
    std::vector<P> out;
    for (const auto& p : pts)
        if (out.empty() || !(out.back().release && p.release)) out.push_back(p);
    return out;
}

template <typename P>
std::vector<Block> blocksOf(const std::vector<P>& pts) {
    std::vector<Block> out;
    for (std::size_t i = 0; i < pts.size(); ++i) {
        if (pts[i].release) continue;
        Block b;
        b.start = i == 0 ? 0.0 : pts[i].beat;
        b.end = i + 1 < pts.size() ? pts[i + 1].beat : kOpenEnd;
        b.value = roundedValue(pts[i]);
        b.point = (int) i;
        out.push_back(b);
    }
    return out;
}

inline int blockAt(const std::vector<Block>& blocks, double beat) {
    for (std::size_t i = 0; i < blocks.size(); ++i)
        if (beat >= blocks[i].start && (blocks[i].open() || beat < blocks[i].end)) return (int) i;
    return -1;
}

template <typename P>
int blockOfPoint(const std::vector<P>& pts, int point) {
    const auto blocks = blocksOf(pts);
    for (std::size_t i = 0; i < blocks.size(); ++i)
        if (blocks[i].point == point || (blocks[i].point + 1 == point && pts[(std::size_t) point].release))
            return (int) i;
    return -1;
}

template <typename P>
int edgeNear(const std::vector<P>& pts, double beat, double tolerance) {
    int best = -1;
    double bestDistance = tolerance;
    for (std::size_t i = 1; i < pts.size(); ++i)
        if (const double d = std::abs(pts[i].beat - beat); d <= bestDistance) {
            bestDistance = d;
            best = (int) i;
        }
    return best;
}

template <typename P>
std::vector<P> painted(const std::vector<P>& pts, double from, double to, int value, int faceplate) {
    from = std::max(0.0, from);
    const bool open = to < 0.0;
    if (!open && to <= from) return pts;
    std::vector<P> out;
    for (const auto& p : pts)
        if (p.beat < from) out.push_back(p);
    if (out.empty() && from > 0.0) out.push_back(stateAt(pts, 0.0, faceplate));
    out.push_back(held<P>(from, value));
    if (!open) {
        out.push_back(stateAt(pts, to, faceplate));
        for (const auto& p : pts)
            if (p.beat > to) out.push_back(p);
    }
    return tidy(out);
}

template <typename P>
std::vector<P> withBlockValue(const std::vector<P>& pts, int block, int value, int faceplate) {
    const auto blocks = blocksOf(pts);
    if (block < 0 || block >= (int) blocks.size()) return pts;
    const auto& b = blocks[(std::size_t) block];
    return painted(pts, b.start, b.end, value, faceplate);
}

template <typename P>
std::vector<P> withoutBlock(std::vector<P> pts, int block, int faceplate) {
    const auto blocks = blocksOf(pts);
    if (block < 0 || block >= (int) blocks.size()) return pts;
    auto& p = pts[(std::size_t) blocks[(std::size_t) block].point];
    p = gap<P>(p.beat, faceplate);
    return tidy(pts);
}

template <typename P>
std::vector<P> withoutEdge(std::vector<P> pts, int index) {
    if (index < 1 || index >= (int) pts.size()) return pts;
    pts.erase(pts.begin() + index);
    return tidy(pts);
}

template <typename P>
std::vector<P> withSplit(const std::vector<P>& pts, double beat) {
    const auto blocks = blocksOf(pts);
    const int b = blockAt(blocks, beat);
    if (b < 0) return pts;
    const auto& block = blocks[(std::size_t) b];
    if (beat <= block.start) return pts;
    std::vector<P> out(pts);
    out.insert(out.begin() + block.point + 1, held<P>(beat, block.value));
    return out;
}

template <typename P>
std::vector<P> withBlockAt(const std::vector<P>& pts, int block, double start, bool keepOriginal, int faceplate) {
    const auto blocks = blocksOf(pts);
    if (block < 0 || block >= (int) blocks.size()) return pts;
    const auto& b = blocks[(std::size_t) block];
    start = std::max(0.0, start);
    const auto base = keepOriginal ? pts : withoutBlock(pts, block, faceplate);
    return painted(base, start, b.open() ? kOpenEnd : start + (b.end - b.start), b.value, faceplate);
}

template <typename P>
std::vector<P> withEdgeAt(std::vector<P> pts, int index, double beat, double minGap) {
    if (index < 1 || index >= (int) pts.size()) return pts;
    const double lo = pts[(std::size_t) index - 1].beat + minGap;
    const double hi = index + 1 < (int) pts.size() ? pts[(std::size_t) index + 1].beat - minGap : beat;
    pts[(std::size_t) index].beat = lo <= hi ? std::clamp(beat, lo, hi) : pts[(std::size_t) index].beat;
    return pts;
}

inline int blockStartingAt(const std::vector<Block>& blocks, double start, double tolerance) {
    for (std::size_t i = 0; i < blocks.size(); ++i)
        if (std::abs(blocks[i].start - start) <= tolerance) return (int) i;
    return -1;
}

template <typename P>
std::vector<Block> blocksStartingAt(const std::vector<P>& pts, const std::vector<double>& starts, double tolerance) {
    std::vector<Block> chosen;
    for (const auto& b : blocksOf(pts))
        if (std::any_of(starts.begin(), starts.end(), [&](double s) { return std::abs(s - b.start) <= tolerance; }))
            chosen.push_back(b);
    return chosen;
}

template <typename P>
std::vector<P> withBlocksMoved(const std::vector<P>& pts, const std::vector<double>& starts, double delta,
                               bool keepOriginals, int faceplate, double tolerance = 1e-6) {
    const auto chosen = blocksStartingAt(pts, starts, tolerance);
    if (chosen.empty()) return pts;
    delta = std::max(delta, -chosen.front().start);
    auto out = pts;
    if (!keepOriginals)
        for (auto it = chosen.rbegin(); it != chosen.rend(); ++it)
            if (const int at = blockStartingAt(blocksOf(out), it->start, tolerance); at >= 0)
                out = withoutBlock(out, at, faceplate);
    for (const auto& b : chosen)
        out = painted(out, b.start + delta, b.open() ? kOpenEnd : b.end + delta, b.value, faceplate);
    return out;
}

template <typename P>
std::vector<P> withBlocksJoined(const std::vector<P>& pts, const std::vector<double>& starts, double tolerance = 1e-6) {
    const auto chosen = blocksStartingAt(pts, starts, tolerance);
    if (chosen.size() < 2) return pts;
    const double from = chosen.front().start, to = chosen.back().start;
    std::vector<P> out;
    for (std::size_t i = 0; i < pts.size(); ++i)
        if (i == 0 || pts[i].beat <= from + tolerance || pts[i].beat > to + tolerance) out.push_back(pts[i]);
    return out;
}

inline std::string labelFor(const std::vector<std::string>& labels, int value) {
    return value >= 0 && value < (int) labels.size() ? labels[(std::size_t) value] : std::to_string(value);
}

}
