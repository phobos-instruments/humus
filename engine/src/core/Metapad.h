#pragma once
#include <cmath>
#include <string>
#include <unordered_map>
#include <vector>

#include "io/PatchDocument.h"

namespace hum {

struct MetaValue {
    std::string organism;
    int propertyIndex = -1;
    double value = 0.0;
    double value2 = 0.0;
    bool isRange = false;
};

namespace metapad_detail {

struct MSPoint { double x = 0.0, y = 0.0; };
using MSPoly = std::vector<MSPoint>;

inline double polyArea(const MSPoly& p) {
    double a = 0.0;
    for (size_t i = 0, n = p.size(); i < n; ++i) {
        const MSPoint& u = p[i];
        const MSPoint& v = p[(i + 1) % n];
        a += u.x * v.y - v.x * u.y;
    }
    return std::abs(a) * 0.5;
}

inline void clipHalfPlane(MSPoly& poly, double a, double b, double c) {
    if (poly.empty()) return;
    MSPoly out;
    out.reserve(poly.size() + 1);
    const double eps = 1e-12;
    const size_t n = poly.size();
    for (size_t i = 0; i < n; ++i) {
        const MSPoint& cur = poly[i];
        const MSPoint& prv = poly[(i + n - 1) % n];
        const double dc = a * cur.x + b * cur.y - c;
        const double dp = a * prv.x + b * prv.y - c;
        const bool curIn = dc <= eps;
        const bool prvIn = dp <= eps;
        if (curIn != prvIn) {
            const double t = dp / (dp - dc);
            out.push_back({prv.x + t * (cur.x - prv.x), prv.y + t * (cur.y - prv.y)});
        }
        if (curIn) out.push_back(cur);
    }
    poly.swap(out);
}

inline void clipCloserTo(MSPoly& poly, double ax, double ay, double bx, double by) {
    const double a = 2.0 * (bx - ax);
    const double b = 2.0 * (by - ay);
    const double c = (bx * bx + by * by) - (ax * ax + ay * ay);
    if (std::abs(a) < 1e-15 && std::abs(b) < 1e-15) return;
    clipHalfPlane(poly, a, b, c);
}

}

inline void shapeWeights(std::vector<double>& w, double temperature) {
    if (temperature == 1.0 || w.empty()) return;
    const double t = std::max(temperature, 1e-3);
    if (t <= 0.05) {
        size_t best = 0;
        for (size_t i = 1; i < w.size(); ++i)
            if (w[i] > w[best]) best = i;
        std::fill(w.begin(), w.end(), 0.0);
        w[best] = 1.0;
        return;
    }
    double sum = 0.0;
    for (auto& v : w) { v = v > 0.0 ? std::pow(v, 1.0 / t) : 0.0; sum += v; }
    if (sum > 1e-12)
        for (auto& v : w) v /= sum;
}

inline std::vector<double> naturalNeighbourWeights(
        const std::vector<MetapadPoint>& pts, double x, double y) {
    using namespace metapad_detail;
    const size_t n = pts.size();
    std::vector<double> w(n, 0.0);
    if (n == 0) return w;
    if (n == 1) { w[0] = 1.0; return w; }

    const MSPoly unitBox = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};

    MSPoly vq = unitBox;
    for (size_t j = 0; j < n; ++j)
        clipCloserTo(vq, x, y, pts[j].x, pts[j].y);
    const double total = polyArea(vq);
    if (total <= 1e-12) {
        size_t best = 0; double bd = 1e300;
        for (size_t i = 0; i < n; ++i) {
            const double dx = pts[i].x - x, dy = pts[i].y - y;
            const double d = dx * dx + dy * dy;
            if (d < bd) { bd = d; best = i; }
        }
        w[best] = 1.0;
        return w;
    }
    for (size_t i = 0; i < n; ++i) {
        MSPoly ci = vq;
        for (size_t j = 0; j < n && !ci.empty(); ++j)
            if (j != i) clipCloserTo(ci, pts[i].x, pts[i].y, pts[j].x, pts[j].y);
        w[i] = polyArea(ci) / total;
    }
    return w;
}

inline std::vector<MetaValue> interpolateMetapad(const MetapadModel& ms,
                                                     double x, double y) {
    std::vector<MetaValue> out;
    if (ms.points.empty() || ms.mask.empty()) return out;

    std::unordered_map<int, const DocumentSnapshot*> byIdx;
    for (auto& s : ms.snapshots) byIdx[s.index] = &s;

    std::vector<MetapadPoint> placedPts;
    std::vector<const DocumentSnapshot*> placedSnap;
    placedPts.reserve(ms.points.size());
    placedSnap.reserve(ms.points.size());
    for (auto& p : ms.points) {
        auto it = byIdx.find(p.snapshotIndex);
        if (it == byIdx.end()) continue;
        placedPts.push_back(p);
        placedSnap.push_back(it->second);
    }
    if (placedPts.empty()) return out;

    auto w = naturalNeighbourWeights(placedPts, x, y);
    shapeWeights(w, ms.temperature);

    auto findVal = [](const DocumentSnapshot* s, const std::string& cn,
                      int pi) -> const SnapshotValue* {
        for (auto& c : s->organisms)
            if (c.organismName == cn)
                for (auto& sv : c.values)
                    if (sv.propertyIndex == pi) return &sv;
        return nullptr;
    };

    for (auto& m : ms.mask) {
        if (!m.restore) continue;
        double acc = 0.0, acc2 = 0.0, wsum = 0.0;
        bool isRange = false;
        for (size_t i = 0; i < placedPts.size(); ++i)
            if (const auto* sv = findVal(placedSnap[i], m.organismName, m.propertyIndex)) {
                const bool r = sv->type == "range";
                isRange = isRange || r;
                acc += w[i] * sv->value;
                acc2 += w[i] * (r ? sv->value2 : sv->value);
                wsum += w[i];
            }
        if (wsum > 1e-9)
            out.push_back({m.organismName, m.propertyIndex, acc / wsum, acc2 / wsum, isRange});
    }
    return out;
}

inline std::unordered_map<int, double> snapshotWeights(const MetapadModel& ms,
                                                       double x, double y) {
    std::unordered_map<int, double> out;
    std::unordered_map<int, const DocumentSnapshot*> byIdx;
    for (auto& s : ms.snapshots) byIdx[s.index] = &s;
    std::vector<MetapadPoint> placed;
    for (auto& p : ms.points)
        if (byIdx.count(p.snapshotIndex) > 0) placed.push_back(p);
    if (placed.empty()) return out;
    auto w = naturalNeighbourWeights(placed, x, y);
    shapeWeights(w, ms.temperature);
    for (size_t i = 0; i < placed.size(); ++i)
        if (w[i] > 0.0) out[placed[i].snapshotIndex] += w[i];
    return out;
}

struct MetaRGB { float r = 0.0f, g = 0.0f, b = 0.0f; };

inline bool parseHexColour(const std::string& hex, MetaRGB& out) {
    if (hex.size() < 7 || hex[0] != '#') return false;
    auto nib = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    int v[6];
    for (int i = 0; i < 6; ++i)
        if ((v[i] = nib(hex[(size_t) i + 1])) < 0) return false;
    out = {(float) (v[0] * 16 + v[1]) / 255.0f, (float) (v[2] * 16 + v[3]) / 255.0f,
           (float) (v[4] * 16 + v[5]) / 255.0f};
    return true;
}

inline MetaRGB metapadFieldColour(const std::vector<MetapadPoint>& pts,
                                      const std::vector<MetaRGB>& cols, double x, double y,
                                      double temperature = 1.0) {
    MetaRGB out;
    auto w = naturalNeighbourWeights(pts, x, y);
    shapeWeights(w, temperature);
    for (size_t i = 0; i < w.size() && i < cols.size(); ++i) {
        out.r += (float) w[i] * cols[i].r;
        out.g += (float) w[i] * cols[i].g;
        out.b += (float) w[i] * cols[i].b;
    }
    return out;
}

}
