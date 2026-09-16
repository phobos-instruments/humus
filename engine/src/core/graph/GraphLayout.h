// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <utility>
#include <vector>

namespace hum {

struct LayoutNode { int width = 116; int height = 38; };
struct LayoutEdge { int src = 0; int dst = 0; int srcPort = 0; int dstPort = 0; };
struct LayoutMetrics {
    int marginX = 24, marginY = 24;
    int hGap = 24, vGap = 48;
};

inline std::vector<std::pair<int, int>> layoutFlowGraph(
    const std::vector<LayoutNode>& nodes, const std::vector<LayoutEdge>& edgesIn,
    const LayoutMetrics& m = {}) {
    const int n = (int) nodes.size();
    std::vector<std::pair<int, int>> pos((size_t) n, {m.marginX, m.marginY});
    if (n == 0) return pos;

    std::vector<LayoutEdge> edges;
    for (const auto& e : edgesIn) {
        if (e.src < 0 || e.src >= n || e.dst < 0 || e.dst >= n || e.src == e.dst) continue;
        const int sp = std::max(0, e.srcPort), dp = std::max(0, e.dstPort);
        bool dup = false;
        for (auto& f : edges)
            if (f.src == e.src && f.dst == e.dst) {
                f.srcPort = std::min(f.srcPort, sp);
                f.dstPort = std::min(f.dstPort, dp);
                dup = true;
                break;
            }
        if (!dup) edges.push_back({e.src, e.dst, sp, dp});
    }

    std::vector<std::vector<int>> out((size_t) n);
    for (int i = 0; i < (int) edges.size(); ++i) out[(size_t) edges[(size_t) i].src].push_back(i);
    enum : int { kUnvisited = 0, kOnStack = 1, kDone = 2 };
    std::vector<int> colour((size_t) n, kUnvisited);
    std::vector<char> back(edges.size(), 0);
    for (int root = 0; root < n; ++root) {
        if (colour[(size_t) root] != kUnvisited) continue;
        std::vector<std::pair<int, size_t>> dfsStack{{root, 0}};
        colour[(size_t) root] = kOnStack;
        while (!dfsStack.empty()) {
            auto& [v, next] = dfsStack.back();
            if (next < out[(size_t) v].size()) {
                const int ei = out[(size_t) v][next++];
                const int w = edges[(size_t) ei].dst;
                if (colour[(size_t) w] == kOnStack) back[(size_t) ei] = 1;
                else if (colour[(size_t) w] == kUnvisited) { colour[(size_t) w] = kOnStack; dfsStack.push_back({w, 0}); }
            } else {
                colour[(size_t) v] = kDone;
                dfsStack.pop_back();
            }
        }
    }

    std::vector<int> layer((size_t) n, 0);
    for (int pass = 0; pass < n; ++pass) {
        bool moved = false;
        for (size_t i = 0; i < edges.size(); ++i) {
            if (back[i]) continue;
            const int want = layer[(size_t) edges[i].src] + 1;
            if (layer[(size_t) edges[i].dst] < want) { layer[(size_t) edges[i].dst] = want; moved = true; }
        }
        if (!moved) break;
    }

    std::vector<char> isolated((size_t) n, 1);
    for (const auto& e : edges) isolated[(size_t) e.src] = isolated[(size_t) e.dst] = 0;

    int numLayers = 1;
    for (int i = 0; i < n; ++i)
        if (!isolated[(size_t) i]) numLayers = std::max(numLayers, layer[(size_t) i] + 1);

    constexpr int kCorridorW = 24;
    std::vector<int> workLayer(layer);
    std::vector<int> workWidth((size_t) n);
    for (int i = 0; i < n; ++i) workWidth[(size_t) i] = nodes[(size_t) i].width;
    std::vector<LayoutEdge> workEdges;
    for (size_t i = 0; i < edges.size(); ++i) {
        const auto& e = edges[i];
        const int span = layer[(size_t) e.dst] - layer[(size_t) e.src];
        if (back[i] || span <= 1) { workEdges.push_back(e); continue; }
        int prev = e.src, prevPort = e.srcPort;
        for (int l = layer[(size_t) e.src] + 1; l < layer[(size_t) e.dst]; ++l) {
            const int v = (int) workLayer.size();
            workLayer.push_back(l);
            workWidth.push_back(kCorridorW);
            workEdges.push_back({prev, v, prevPort, 0});
            prev = v;
            prevPort = 0;
        }
        workEdges.push_back({prev, e.dst, prevPort, e.dstPort});
    }
    const int workCount = (int) workLayer.size();
    auto isVirtual = [&](int v) { return v >= n; };
    auto inFlow = [&](int v) { return isVirtual(v) || !isolated[(size_t) v]; };

    std::vector<std::vector<int>> rows((size_t) numLayers);
    for (int i = 0; i < workCount; ++i)
        if (inFlow(i)) rows[(size_t) workLayer[(size_t) i]].push_back(i);

    const double kPortBias = 0.4;
    std::vector<int> inSpan((size_t) workCount, 0), outSpan((size_t) workCount, 0);
    for (const auto& e : workEdges) {
        inSpan[(size_t) e.dst] = std::max(inSpan[(size_t) e.dst], e.dstPort);
        outSpan[(size_t) e.src] = std::max(outSpan[(size_t) e.src], e.srcPort);
    }
    auto nudge = [&](int port, int span) {
        return span <= 0 ? 0.0 : ((port + 0.5) / (span + 1.0) - 0.5) * 2.0 * kPortBias;
    };
    std::vector<int> slot((size_t) workCount, 0);
    auto reslot = [&] {
        for (auto& row : rows)
            for (int k = 0; k < (int) row.size(); ++k) slot[(size_t) row[(size_t) k]] = k;
    };
    reslot();
    auto sweep = [&](bool down) {
        for (int r = 0; r < numLayers; ++r) {
            auto& row = rows[(size_t) (down ? r : numLayers - 1 - r)];
            std::vector<std::pair<double, int>> keyed;
            for (int v : row) {
                double sum = 0.0; int cnt = 0;
                for (const auto& e : workEdges) {
                    if (e.dst == v && workLayer[(size_t) e.src] != workLayer[(size_t) v] && inFlow(e.src)) {
                        sum += slot[(size_t) e.src] + nudge(e.srcPort, outSpan[(size_t) e.src]);
                        ++cnt;
                    }
                    if (e.src == v && workLayer[(size_t) e.dst] != workLayer[(size_t) v] && inFlow(e.dst)) {
                        sum += slot[(size_t) e.dst] + nudge(e.dstPort, inSpan[(size_t) e.dst]);
                        ++cnt;
                    }
                }
                keyed.push_back({cnt > 0 ? sum / cnt : (double) slot[(size_t) v], v});
            }
            std::stable_sort(keyed.begin(), keyed.end(),
                             [](const auto& a, const auto& b) { return a.first < b.first; });
            for (size_t k = 0; k < keyed.size(); ++k) row[k] = keyed[k].second;
            reslot();
        }
    };
    for (int i = 0; i < 3; ++i) { sweep(true); sweep(false); }

    int widest = 0;
    std::vector<int> rowWidth((size_t) numLayers, 0);
    for (int r = 0; r < numLayers; ++r) {
        int w = 0;
        for (int v : rows[(size_t) r]) w += workWidth[(size_t) v] + m.hGap;
        rowWidth[(size_t) r] = std::max(0, w - m.hGap);
        widest = std::max(widest, rowWidth[(size_t) r]);
    }
    std::vector<double> workX((size_t) workCount, (double) m.marginX);
    for (int r = 0; r < numLayers; ++r) {
        double x = m.marginX + (widest - rowWidth[(size_t) r]) / 2.0;
        for (int v : rows[(size_t) r]) {
            workX[(size_t) v] = x;
            x += workWidth[(size_t) v] + m.hGap;
        }
    }

    auto centreOf = [&](int v) { return workX[(size_t) v] + workWidth[(size_t) v] / 2.0; };
    auto straighten = [&](bool fromAbove) {
        for (int rr = 0; rr < numLayers; ++rr) {
            const int r = fromAbove ? rr : numLayers - 1 - rr;
            auto& row = rows[(size_t) r];
            if (row.empty()) continue;
            std::vector<double> want(row.size()), prio(row.size());
            for (size_t k = 0; k < row.size(); ++k) {
                const int v = row[k];
                double sum = 0.0, cnt = 0.0;
                for (const auto& e : workEdges) {
                    const int other = e.dst == v ? e.src : e.src == v ? e.dst : -1;
                    if (other < 0 || !inFlow(other)) continue;
                    const int dl = workLayer[(size_t) other] - r;
                    if (dl == -1 || dl == 1) {
                        const double w = isVirtual(v) || isVirtual(other) ? 4.0 : 1.0;
                        sum += w * centreOf(other);
                        cnt += w;
                    }
                }
                want[k] = cnt > 0.0 ? sum / cnt - workWidth[(size_t) v] / 2.0 : workX[(size_t) v];
                prio[k] = isVirtual(v) ? 1e6 : cnt;
            }
            std::vector<size_t> byPrio(row.size());
            std::iota(byPrio.begin(), byPrio.end(), (size_t) 0);
            std::stable_sort(byPrio.begin(), byPrio.end(),
                             [&](size_t a, size_t b) { return prio[a] > prio[b]; });
            std::vector<char> placed(row.size(), 0);
            for (const size_t k : byPrio) {
                double lo = -1e18, hi = 1e18, need = 0.0;
                for (size_t j = k; j-- > 0;) {
                    if (placed[j]) {
                        lo = workX[(size_t) row[j]] + workWidth[(size_t) row[j]] + m.hGap + need;
                        break;
                    }
                    need += workWidth[(size_t) row[j]] + m.hGap;
                }
                need = 0.0;
                for (size_t j = k + 1; j < row.size(); ++j) {
                    if (placed[j]) {
                        hi = workX[(size_t) row[j]] - need - workWidth[(size_t) row[k]] - m.hGap;
                        break;
                    }
                    need += workWidth[(size_t) row[j]] + m.hGap;
                }
                workX[(size_t) row[k]] = std::clamp(want[k], lo, std::max(lo, hi));
                placed[k] = 1;
            }
        }
    };
    for (int i = 0; i < 8; ++i) { straighten(true); straighten(false); }

    double minX = 1e18;
    for (int v = 0; v < workCount; ++v)
        if (inFlow(v)) minX = std::min(minX, workX[(size_t) v]);
    if (minX > 1e17) minX = m.marginX;
    int y = m.marginY;
    for (int r = 0; r < numLayers; ++r) {
        int rowH = 0;
        for (int v : rows[(size_t) r]) {
            if (!isVirtual(v))
                pos[(size_t) v] = {(int) std::lround(workX[(size_t) v] - minX) + m.marginX, y};
            rowH = std::max(rowH, isVirtual(v) ? 0 : nodes[(size_t) v].height);
        }
        y += (rowH > 0 ? rowH : 38) + m.vGap;
    }

    {
        auto bez = [&](double a, double c1v, double c2v, double b, double t) {
            const double u = 1.0 - t;
            return u * u * u * a + 3 * u * u * t * c1v + 3 * u * t * t * c2v + t * t * t * b;
        };
        for (int round = 0; round < 6; ++round) {
            bool pushed = false;
            for (const auto& e : edges) {
                if (isolated[(size_t) e.src] || isolated[(size_t) e.dst]) continue;
                const double ax = pos[(size_t) e.src].first + nodes[(size_t) e.src].width / 2.0;
                const double ay = pos[(size_t) e.src].second + nodes[(size_t) e.src].height;
                const double bx = pos[(size_t) e.dst].first + nodes[(size_t) e.dst].width / 2.0;
                const double by = pos[(size_t) e.dst].second;
                for (int b = 0; b < n; ++b) {
                    if (b == e.src || b == e.dst || isolated[(size_t) b]) continue;
                    const double bl = pos[(size_t) b].first, bt = pos[(size_t) b].second;
                    const double br = bl + nodes[(size_t) b].width;
                    const double bb = bt + nodes[(size_t) b].height;
                    double lo = 1e18, hi = -1e18;
                    bool hit = false;
                    for (int i = 0; i <= 64; ++i) {
                        const double t = i / 64.0;
                        const double yb = bez(ay, ay + 30, by - 30, by, t);
                        if (yb < bt || yb > bb) continue;
                        const double xb = bez(ax, ax, bx, bx, t);
                        lo = std::min(lo, xb);
                        hi = std::max(hi, xb);
                        hit = hit || (xb > bl && xb < br);
                    }
                    if (!hit) continue;
                    const double pushR = (hi + m.hGap) - bl;
                    const double pushL = br - (lo - m.hGap);
                    const int dx = (int) std::lround(pushR <= pushL ? pushR : -pushL);
                    for (auto& row : rows) {
                        if (std::find(row.begin(), row.end(), b) == row.end()) continue;
                        pos[(size_t) b].first += dx;
                        std::vector<int> real;
                        for (int v : row)
                            if (!isVirtual(v)) real.push_back(v);
                        const auto at = std::find(real.begin(), real.end(), b);
                        if (dx > 0) {
                            for (auto j = at + 1; j != real.end(); ++j) {
                                const int need = pos[(size_t) *(j - 1)].first
                                               + nodes[(size_t) *(j - 1)].width + m.hGap;
                                if (pos[(size_t) *j].first < need) pos[(size_t) *j].first = need;
                            }
                        } else {
                            for (auto j = at; j != real.begin();) {
                                --j;
                                const int cap = pos[(size_t) *(j + 1)].first
                                              - nodes[(size_t) *j].width - m.hGap;
                                if (pos[(size_t) *j].first > cap) pos[(size_t) *j].first = cap;
                            }
                        }
                        break;
                    }
                    pushed = true;
                }
            }
            if (!pushed) break;
        }
        int flowMin = m.marginX;
        for (int i = 0; i < n; ++i)
            if (!isolated[(size_t) i]) flowMin = std::min(flowMin, pos[(size_t) i].first);
        if (flowMin < m.marginX)
            for (int i = 0; i < n; ++i)
                if (!isolated[(size_t) i]) pos[(size_t) i].first += m.marginX - flowMin;
    }

    const int wrapAt = std::max(widest, 480);
    int x = m.marginX, rowH = 0;
    for (int i = 0; i < n; ++i) {
        if (!isolated[(size_t) i]) continue;
        if (x > m.marginX && x + nodes[(size_t) i].width > m.marginX + wrapAt) {
            x = m.marginX;
            y += rowH + m.vGap / 2;
            rowH = 0;
        }
        pos[(size_t) i] = {x, y};
        x += nodes[(size_t) i].width + m.hGap;
        rowH = std::max(rowH, nodes[(size_t) i].height);
    }
    return pos;
}

}
