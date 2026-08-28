#pragma once
#include <algorithm>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/Automation.h"
#include "core/ParamSchema.h"
#include "io/PatchDocument.h"

namespace hum::envpaint {

inline std::pair<double, double> laneRange(const OrganismModel* cm,
                                           const std::string& param,
                                           int metaSnapshots = 0) {
    if (cm == nullptr) return {0.0, 1.0};
    if (isClockPseudo(cm->displayClass) && param == kTempoParam)
        return {kTempoLaneMin, kTempoLaneMax};
    if (isMetapadPseudo(cm->displayClass) && param == kMetaRecallParam)
        return {0.0, (double) std::max(1, metaSnapshots)};
    for (const auto& d : schemaFor(cm->classRaw))
        if (d.name == param) return {d.min, d.isRange ? d.defMax : d.max};
    return {0.0, 1.0};
}

inline void draw(juce::Graphics& g, const std::vector<AutomationBreakpoint>& pts,
                 const std::string& kind, float leftX, float rowY, float rowH,
                 const std::function<float(double)>& xFor,
                 const std::function<float(double)>& yFor, juce::Colour line,
                 float dotR = 3.0f) {
    if (pts.empty()) return;
    if (kind == "trigger") {
        for (const auto& p : pts) {
            const float x = xFor(p.beat);
            g.setColour(line);
            g.fillRect(x - 1.0f, rowY + 8.0f, 2.0f, rowH - 16.0f);
            g.fillEllipse(x - dotR, rowY + 8.0f, dotR * 2.0f, dotR * 2.0f);
        }
    } else if (kind == "range") {
        juce::Path lo, hi, band;
        const int n = (int) pts.size();
        for (int k = 0; k < n; ++k) {
            const float x = xFor(pts[(size_t) k].beat);
            const float yl = yFor(pts[(size_t) k].value);
            const float yh = yFor(pts[(size_t) k].valueMax);
            if (k == 0) { lo.startNewSubPath(leftX, yl); hi.startNewSubPath(leftX, yh);
                          band.startNewSubPath(leftX, yh); }
            lo.lineTo(x, yl); hi.lineTo(x, yh); band.lineTo(x, yh);
        }
        for (int k = n - 1; k >= 0; --k)
            band.lineTo(xFor(pts[(size_t) k].beat), yFor(pts[(size_t) k].value));
        band.lineTo(leftX, yFor(pts.front().value));
        band.closeSubPath();
        g.setColour(line.withAlpha(0.16f));
        g.fillPath(band);
        g.setColour(line);
        g.strokePath(lo, juce::PathStrokeType(1.4f));
        g.strokePath(hi, juce::PathStrokeType(1.4f));
        for (const auto& p : pts) {
            const float x = xFor(p.beat);
            g.fillEllipse(x - dotR, yFor(p.value) - dotR, dotR * 2.0f, dotR * 2.0f);
            g.fillEllipse(x - dotR, yFor(p.valueMax) - dotR, dotR * 2.0f, dotR * 2.0f);
        }
    } else {
        juce::Path path;
        for (int k = 0; k < (int) pts.size(); ++k) {
            const float x = xFor(pts[(size_t) k].beat);
            const float y = yFor(pts[(size_t) k].value);
            if (k == 0) { path.startNewSubPath(leftX, y); path.lineTo(x, y); continue; }
            const auto& a = pts[(size_t) (k - 1)];
            if (a.curve == 0.0) { path.lineTo(x, y); continue; }
            const float ax = xFor(a.beat);
            const int steps = juce::jlimit(4, 48, (int) std::abs(x - ax) / 3);
            for (int i = 1; i <= steps; ++i) {
                const double t = (double) i / steps;
                path.lineTo(ax + (x - ax) * (float) t,
                            yFor(a.value + (pts[(size_t) k].value - a.value)
                                               * shapeT(t, a.curve)));
            }
        }
        g.setColour(line);
        g.strokePath(path, juce::PathStrokeType(1.6f));
        for (const auto& p : pts) {
            const float x = xFor(p.beat), y = yFor(p.value);
            g.fillEllipse(x - dotR, y - dotR, dotR * 2.0f, dotR * 2.0f);
        }
    }
}

}
