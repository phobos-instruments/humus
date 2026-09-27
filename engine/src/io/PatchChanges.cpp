// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "io/PatchChanges.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <map>
#include <tuple>

namespace hum::history {

namespace {

using CordKey = std::tuple<int, std::string, int, std::string, int, int>;

std::map<CordKey, int> cordsOf(const PatchDocumentModel& doc) {
    std::map<CordKey, int> out;
    int family = 0;
    for (const auto* list : {&doc.connections, &doc.midiConnections, &doc.videoConnections}) {
        for (const auto& c : *list) ++out[{family, c.src, c.srcOutlet, c.dst, c.dstInlet, c.midiChannel}];
        ++family;
    }
    return out;
}

bool sameNumber(double a, double b) { return std::abs(a - b) <= 1e-9; }

bool sameParameter(const Parameter& a, const Parameter& b) {
    return sameNumber(a.value, b.value) && a.text == b.text && a.isRange == b.isRange
           && sameNumber(a.rangeMin, b.rangeMin) && sameNumber(a.rangeMax, b.rangeMax);
}

bool sameChannel(const PatternChannel& a, const PatternChannel& b) {
    return a.type == b.type && a.triggers == b.triggers && a.matrix == b.matrix && a.audioFile == b.audioFile
           && a.startTick == b.startTick && a.lengthTicks == b.lengthTicks && a.audioOffset == b.audioOffset
           && sameNumber(a.audioGain, b.audioGain) && a.loopClip == b.loopClip;
}

bool samePattern(const Pattern& a, const Pattern& b) {
    if (a.channels.size() != b.channels.size() || a.duration != b.duration) return false;
    for (std::size_t i = 0; i < a.channels.size(); ++i)
        if (!sameChannel(a.channels[i], b.channels[i])) return false;
    return true;
}

bool sameAutomation(const std::vector<AutomationLane>& a, const std::vector<AutomationLane>& b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a[i].propertyName != b[i].propertyName || a[i].mute != b[i].mute
            || a[i].points.size() != b[i].points.size())
            return false;
        for (std::size_t p = 0; p < a[i].points.size(); ++p) {
            const auto& x = a[i].points[p];
            const auto& y = b[i].points[p];
            if (!sameNumber(x.beat, y.beat) || !sameNumber(x.value, y.value) || !sameNumber(x.curve, y.curve))
                return false;
        }
    }
    return true;
}

void compareOrganism(const OrganismModel& was, const OrganismModel& now, std::vector<Change>& settings,
                     std::vector<Change>& patterns) {
    if (!samePattern(was.pattern, now.pattern)) patterns.push_back({Change::Kind::Pattern, now.name, {}, 0});
    if (!sameAutomation(was.automation, now.automation))
        patterns.push_back({Change::Kind::Automation, now.name, {}, 0});
    for (const auto& p : now.properties) {
        const Parameter* old = nullptr;
        for (const auto& q : was.properties)
            if (q.name == p.name) old = &q;
        if (p.type == "pattern") continue;
        if (old == nullptr || !sameParameter(*old, p)) settings.push_back({Change::Kind::Setting, now.name, p.name, 0});
    }
}

}

std::vector<Change> changesBetween(const PatchDocumentModel& before, const PatchDocumentModel& after) {
    std::vector<Change> added, removed, cords, tempo, patterns, settings;
    for (const auto& cm : after.organisms) {
        if (cm.internal) continue;
        const auto* was = before.byName(cm.name);
        if (was == nullptr) added.push_back({Change::Kind::Added, cm.name, cm.displayClass, 0});
        else compareOrganism(*was, cm, settings, patterns);
    }
    for (const auto& cm : before.organisms)
        if (!cm.internal && after.byName(cm.name) == nullptr)
            removed.push_back({Change::Kind::Removed, cm.name, cm.displayClass, 0});

    const auto was = cordsOf(before), now = cordsOf(after);
    int plus = 0, minus = 0;
    for (const auto& [key, n] : now) {
        const auto it = was.find(key);
        plus += std::max(0, n - (it == was.end() ? 0 : it->second));
    }
    for (const auto& [key, n] : was) {
        const auto it = now.find(key);
        minus += std::max(0, n - (it == now.end() ? 0 : it->second));
    }
    if (plus > 0) cords.push_back({Change::Kind::CordsAdded, {}, {}, plus});
    if (minus > 0) cords.push_back({Change::Kind::CordsRemoved, {}, {}, minus});
    if (!sameNumber(before.clock.tempo, after.clock.tempo))
        tempo.push_back({Change::Kind::Tempo, {}, std::to_string((int) std::lround(after.clock.tempo)), 0});

    std::vector<Change> out;
    for (auto* group : {&added, &removed, &cords, &tempo, &patterns, &settings})
        out.insert(out.end(), group->begin(), group->end());
    return out;
}

std::string phraseOf(const Change& c) {
    switch (c.kind) {
        case Change::Kind::Added: return "added " + c.organism;
        case Change::Kind::Removed: return "removed " + c.organism;
        case Change::Kind::CordsAdded: return std::to_string(c.count) + (c.count == 1 ? " cord" : " cords") + " added";
        case Change::Kind::CordsRemoved:
            return std::to_string(c.count) + (c.count == 1 ? " cord" : " cords") + " removed";
        case Change::Kind::Tempo: return "tempo " + c.detail;
        case Change::Kind::Pattern: return c.organism + " notes or clips";
        case Change::Kind::Automation: return c.organism + " automation";
        case Change::Kind::Setting: return c.organism + " " + c.detail;
    }
    return {};
}

std::string summaryOf(const std::vector<Change>& changes, std::size_t shown) {
    std::string out;
    for (std::size_t i = 0; i < changes.size() && i < shown; ++i) {
        if (!out.empty()) out += ", ";
        out += phraseOf(changes[i]);
    }
    if (changes.size() > shown) out += " and " + std::to_string(changes.size() - shown) + " more";
    return out;
}

}
