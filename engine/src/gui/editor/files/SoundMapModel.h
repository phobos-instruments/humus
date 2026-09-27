// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

#include "gui/editor/LiveControls.h"
#include "gui/editor/files/GrainTrail.h"
#include "gui/host/ModelHost.h"
#include "hum/caps/Files.h"
#include "hum/caps/Samples.h"

namespace hum::files {

inline constexpr double kMapLeastSpan = 0.25;
inline constexpr double kMapMargin = 0.08;
inline constexpr int kGrainPull = 128;

struct MapWindow {
    double x0 = 0.0, x1 = 1.0, y0 = 0.0, y1 = 1.0;

    double paramX(double frac) const { return x0 + frac * (x1 - x0); }
    double paramY(double frac) const { return y0 + frac * (y1 - y0); }
    double viewX(double value) const { return x1 > x0 ? (value - x0) / (x1 - x0) : value; }
    double viewY(double value) const { return y1 > y0 ? (value - y0) / (y1 - y0) : value; }
};

inline std::pair<double, double> mapSpan(double lo, double hi) {
    lo -= kMapMargin;
    hi += kMapMargin;
    if (hi - lo < kMapLeastSpan) {
        const double mid = 0.5 * (lo + hi);
        lo = mid - kMapLeastSpan * 0.5;
        hi = mid + kMapLeastSpan * 0.5;
    }
    if (lo < 0.0) { hi = std::min(1.0, hi - lo); lo = 0.0; }
    if (hi > 1.0) { lo = std::max(0.0, lo - (hi - 1.0)); hi = 1.0; }
    return {lo, hi};
}

inline MapWindow mapWindow(const std::vector<SoundMapSource::MapPoint>& points, bool fit) {
    MapWindow out;
    if (!fit || points.size() < 2) return out;
    double lox = 1.0, hix = 0.0, loy = 1.0, hiy = 0.0;
    for (const auto& p : points) {
        lox = std::min(lox, (double) p.x);
        hix = std::max(hix, (double) p.x);
        loy = std::min(loy, (double) p.y);
        hiy = std::max(hiy, (double) p.y);
    }
    if (hix < lox || hiy < loy) return out;
    const auto sx = mapSpan(lox, hix);
    const auto sy = mapSpan(loy, hiy);
    out.x0 = sx.first; out.x1 = sx.second;
    out.y0 = sy.first; out.y1 = sy.second;
    return out;
}

class SoundMapModel {
public:
    SoundMapModel(ModelHost& host, std::string organism, std::string paramX, std::string paramY, std::string spray,
                  std::string filePrefix, std::string fit)
        : host_(host), organism_(std::move(organism)), px_(std::move(paramX)), py_(std::move(paramY)),
          spray_(std::move(spray)), filePrefix_(std::move(filePrefix)), fit_(std::move(fit)) {}

    const MapWindow& window() const { return window_; }

    const std::string& paramX() const { return px_; }
    const std::string& paramY() const { return py_; }
    const std::vector<SoundMapSource::MapPoint>& points() const { return points_; }
    double x() const { return host_.liveParamValue(organism_, px_); }
    double y() const { return host_.liveParamValue(organism_, py_); }
    double spray() const { return host_.liveParamValue(organism_, spray_); }

    std::string readout() const {
        return std::to_string((int) std::lround(x() * 100.0)) + " , " + std::to_string((int) std::lround(y() * 100.0));
    }

    void invalidate() { generation_ = ~0u; }

    bool refreshPoints(bool force) {
        auto* src = live::source<SoundMapSource>(host_, organism_);
        const unsigned gen = src ? src->mapGeneration() : 0;
        if (!force && gen == generation_) return false;
        generation_ = gen;
        points_ = src ? src->mapPoints() : std::vector<SoundMapSource::MapPoint>{};
        window_ = mapWindow(points_, !fit_.empty() && host_.liveParamValue(organism_, fit_) >= 0.5);
        return true;
    }

    void pullGrains(double now) {
        auto* src = live::source<GrainFlashSource>(host_, organism_);
        if (src == nullptr) {
            trail_.forget();
            return;
        }
        std::array<GrainFlashSource::Flash, kGrainPull> got;
        trail_.take(got.data(), src->recentGrains(got.data(), (int) got.size()), now);
    }
    const GrainTrail& trail() const { return trail_; }

    void begin() {}
    void end() {}

    void place(float px, float py, int width, int height, bool) {
        const double fx = window_.paramX(std::clamp(px / (double) std::max(1, width), 0.0, 1.0));
        const double fy = window_.paramY(std::clamp(1.0 - py / (double) std::max(1, height), 0.0, 1.0));
        host_.setParam(organism_, px_, fx);
        host_.setParam(organism_, py_, fy);
    }

    bool collectTakes() {
        auto* cap = live::source<LiveCaptureSource>(host_, organism_);
        if (cap == nullptr) return false;
        LiveCaptureSource::Take take;
        if (!cap->fetchCompletedTake(take)) return false;
        host_.setParamText(organism_, filePrefix_ + std::to_string(take.slot), take.path);
        return true;
    }

private:
    MapWindow window_;

    ModelHost& host_;
    std::string organism_, px_, py_, spray_, filePrefix_, fit_;
    std::vector<SoundMapSource::MapPoint> points_;
    unsigned generation_ = ~0u;
    GrainTrail trail_;
};

}
