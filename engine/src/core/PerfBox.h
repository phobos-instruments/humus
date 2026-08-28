#pragma once
#include <algorithm>
#include <string>
#include <vector>

#include "io/PatchDocument.h"

namespace hum::perfbox {

inline void addSpan(std::vector<PerformanceBox>& boxes, const std::string& organism,
                    double s, double e) {
    if (e < s) std::swap(s, e);
    for (auto it = boxes.begin(); it != boxes.end();) {
        if (it->organism == organism && it->startBeat <= e && it->endBeat >= s) {
            s = std::min(s, it->startBeat);
            e = std::max(e, it->endBeat);
            it = boxes.erase(it);
        } else {
            ++it;
        }
    }
    boxes.push_back({organism, s, e});
}

inline void derive(PatchDocumentModel& doc) {
    if (!doc.perfBoxes.empty()) return;
    for (const auto& c : doc.organisms) {
        double lo = 1e18, hi = -1e18;
        for (const auto& l : c.automation)
            for (const auto& p : l.points) {
                lo = std::min(lo, p.beat);
                hi = std::max(hi, p.beat);
            }
        if (hi < lo) continue;
        addSpan(doc.perfBoxes, c.name, lo, std::max(hi, lo + 1.0));
    }
}

namespace detail {
inline void erasePoints(PatchDocumentModel& doc, const std::string& organism,
                        double s, double e) {
    if (auto* c = doc.byName(organism))
        for (auto& l : const_cast<OrganismModel*>(c)->automation)
            l.points.erase(std::remove_if(l.points.begin(), l.points.end(),
                           [&](const AutomationBreakpoint& b) {
                               return b.beat >= s && b.beat <= e;
                           }), l.points.end());
}
inline void sortLanes(PatchDocumentModel& doc, const std::string& organism) {
    if (auto* c = doc.byName(organism))
        for (auto& l : const_cast<OrganismModel*>(c)->automation)
            std::sort(l.points.begin(), l.points.end(),
                      [](const AutomationBreakpoint& a, const AutomationBreakpoint& b) {
                          return a.beat < b.beat;
                      });
}
}

inline void moveBox(PatchDocumentModel& doc, int index, double delta) {
    if (index < 0 || index >= (int) doc.perfBoxes.size()) return;
    auto& box = doc.perfBoxes[(size_t) index];
    delta = std::max(delta, -box.startBeat);
    if (delta == 0.0) return;
    auto* c = const_cast<OrganismModel*>(doc.byName(box.organism));
    if (c != nullptr) {
        for (auto& l : c->automation) {
            std::vector<AutomationBreakpoint> moving;
            l.points.erase(std::remove_if(l.points.begin(), l.points.end(),
                           [&](const AutomationBreakpoint& b) {
                               const bool in = b.beat >= box.startBeat && b.beat <= box.endBeat;
                               if (in) moving.push_back(b);
                               return in;
                           }), l.points.end());
            l.points.erase(std::remove_if(l.points.begin(), l.points.end(),
                           [&](const AutomationBreakpoint& b) {
                               return b.beat >= box.startBeat + delta
                                   && b.beat <= box.endBeat + delta;
                           }), l.points.end());
            for (auto p : moving) { p.beat += delta; l.points.push_back(p); }
        }
        detail::sortLanes(doc, box.organism);
    }
    box.startBeat += delta;
    box.endBeat += delta;
}

inline int duplicateBox(PatchDocumentModel& doc, int index, double atBeat) {
    if (index < 0 || index >= (int) doc.perfBoxes.size()) return -1;
    const auto box = doc.perfBoxes[(size_t) index];
    const double delta = std::max(0.0, atBeat) - box.startBeat;
    if (delta == 0.0) return index;
    auto* c = const_cast<OrganismModel*>(doc.byName(box.organism));
    if (c != nullptr) {
        for (auto& l : c->automation) {
            std::vector<AutomationBreakpoint> copies;
            for (const auto& b : l.points)
                if (b.beat >= box.startBeat && b.beat <= box.endBeat) copies.push_back(b);
            l.points.erase(std::remove_if(l.points.begin(), l.points.end(),
                           [&](const AutomationBreakpoint& b) {
                               return b.beat >= box.startBeat + delta
                                   && b.beat <= box.endBeat + delta
                                   && !(b.beat >= box.startBeat && b.beat <= box.endBeat);
                           }), l.points.end());
            for (auto p : copies) { p.beat += delta; l.points.push_back(p); }
        }
        detail::sortLanes(doc, box.organism);
    }
    addSpan(doc.perfBoxes, box.organism, box.startBeat + delta, box.endBeat + delta);
    return (int) doc.perfBoxes.size() - 1;
}

inline void trimBox(PatchDocumentModel& doc, int index, double newStart, double newEnd) {
    if (index < 0 || index >= (int) doc.perfBoxes.size()) return;
    auto& box = doc.perfBoxes[(size_t) index];
    newStart = std::max(0.0, newStart);
    newEnd = std::max(newStart + 0.25, newEnd);
    if (newStart > box.startBeat)
        detail::erasePoints(doc, box.organism, box.startBeat, newStart - 1e-9);
    if (newEnd < box.endBeat)
        detail::erasePoints(doc, box.organism, newEnd + 1e-9, box.endBeat);
    box.startBeat = newStart;
    box.endBeat = newEnd;
}

inline int splitBox(PatchDocumentModel& doc, int index, double atBeat) {
    if (index < 0 || index >= (int) doc.perfBoxes.size()) return -1;
    auto& box = doc.perfBoxes[(size_t) index];
    if (atBeat < box.startBeat + 0.25 || atBeat > box.endBeat - 0.25) return -1;
    const double e = box.endBeat;
    box.endBeat = atBeat;
    doc.perfBoxes.push_back({box.organism, atBeat, e});
    return (int) doc.perfBoxes.size() - 1;
}

inline void deleteBox(PatchDocumentModel& doc, int index) {
    if (index < 0 || index >= (int) doc.perfBoxes.size()) return;
    const auto box = doc.perfBoxes[(size_t) index];
    detail::erasePoints(doc, box.organism, box.startBeat, box.endBeat);
    doc.perfBoxes.erase(doc.perfBoxes.begin() + index);
}

}
