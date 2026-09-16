// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/packs/Categories.h"

#include <algorithm>
#include <unordered_map>

#include "core/packs/ClassString.h"
#include "core/packs/PackRegistry.h"
#include "core/plugins/PluginHost.h"
#include "core/packs/Roles.h"

namespace hum {

bool isHiddenOrganism(const std::string& c) {
    return c == "ClockPseudoSP" || c == "MetasurfacePseudoSP" || classHasRole(c, role::kMidiTrack);
}

std::string pseudoOwnerLabel(const std::string& c) {
    if (c == "ClockPseudoSP") return "Transport";
    if (c == "MetasurfacePseudoSP") return "Metapad";
    return {};
}

std::string canonicalClass(const std::string& className) {
    if (const auto* m = PackRegistry::instance().classManifest(className))
        if (!m->canonical.empty()) return m->canonical;
    return className;
}

std::string categoryOf(const std::string& c) {
    if (isHiddenOrganism(c)) return "";

    if (const auto id = parseClassString(c); isPluginKind(id.kind)) {
        auto& ph = PluginHost::instance();
        const std::string fmt = id.kind == "au" ? "Audio Units" : id.kind == "lv2" ? "LV2" : "VST3";
        return fmt + " · " + (ph.isInstrument(c) ? std::string("Instruments · ")   // utf8-ok: data
                                                 : std::string("Effects · ")) + ph.vendorOf(c);   // utf8-ok: data
    }

    if (c == "PodIn" || c == "PodOut" || c == "SPodIn" || c == "SPodOut"
        || c == "PodMidiIn" || c == "PodMidiOut" || c == "PodVideoIn" || c == "PodVideoOut"
        || c == "PodControlIn" || c == "PodControlOut")
        return "Pod";
    if (c == "PodInlet" || c == "PodOutlet" || c == "SPodInlet" || c == "SPodOutlet"
        || c == "PodMidiInlet" || c == "PodMidiOutlet")
        return "";

    if (const auto* m = PackRegistry::instance().classManifest(c))
        return m->hidden ? std::string{} : m->category;

    return "Other";
}

std::string originOf(const std::string& c) {
    if (isPluginKind(parseClassString(c).kind)) return "Plugins";
    if (c == "PodIn" || c == "PodOut" || c == "SPodIn" || c == "SPodOut"
        || c == "PodMidiIn" || c == "PodMidiOut"
        || c == "PodVideoIn" || c == "PodVideoOut"
        || c == "PodControlIn" || c == "PodControlOut"
        || c == "PodInlet" || c == "PodOutlet" || c == "SPodInlet" || c == "SPodOutlet"
        || c == "PodMidiInlet" || c == "PodMidiOutlet")
        return "Humus";
    if (const auto* p = PackRegistry::instance().packOf(c)) return p->manifest.origin;
    return "Other";
}

std::pair<std::string, std::string> splitCategory(const std::string& category) {
    const auto sep = category.find(" \xc2\xb7 ");   // utf8-ok: data
    if (sep == std::string::npos) return {category, ""};
    return {category.substr(0, sep), category.substr(sep + 4)};
}

std::vector<std::string> categorySegments(const std::string& category) {
    std::vector<std::string> segs;
    size_t pos = 0;
    while (true) {
        const auto sep = category.find(" \xc2\xb7 ", pos);   // utf8-ok: data
        if (sep == std::string::npos) {
            segs.push_back(category.substr(pos));
            return segs;
        }
        segs.push_back(category.substr(pos, sep - pos));
        pos = sep + 4;
    }
}

Family familyOf(const std::string& className) {
    auto cat = categoryOf(className);
    if (cat.empty()) {
        if (const auto* m = PackRegistry::instance().classManifest(className))
            cat = m->category;
        if (cat.empty()) return Family::Utility;
    }

    if (cat.find(" \xc2\xb7 ") != std::string::npos)   // utf8-ok: data
        return cat.find("Instruments") != std::string::npos ? Family::Voice : Family::Time;

    if (cat == "Instruments" || cat == "Signal Generators" || cat == "Players")
        return Family::Voice;
    if (cat == "Effects" || cat == "Filters" || cat == "Spectral" || cat == "Dynamics")
        return Family::Time;
    if (cat == "Control" || cat == "Primitives" || cat == "Sequencers")
        return Family::Motion;
    if (cat == "MIDI Inputs" || cat == "MIDI Outputs" || cat == "Visual"
        || cat == "Meters")
        return Family::Sense;
    return Family::Utility;
}

std::string familyName(Family f) {
    switch (f) {
        case Family::Voice: return "Voice";
        case Family::Time: return "Time";
        case Family::Motion: return "Motion";
        case Family::Sense: return "Sense";
        case Family::Utility: break;
    }
    return "Utility";
}

CategoryGroups
groupByCategory(const std::vector<std::string>& classNames) {
    std::unordered_map<std::string, std::vector<std::string>> byCat;
    for (const auto& c : classNames) {
        auto cat = categoryOf(c);
        if (cat.empty()) continue;
        byCat[cat].push_back(c);
    }
    std::vector<std::string> cats;
    cats.reserve(byCat.size());
    for (const auto& kv : byCat) cats.push_back(kv.first);

    auto rank = [](const std::string& s) {
        if (s.find("Instruments \xc2\xb7 ") != std::string::npos) return 0;   // utf8-ok: data
        if (s.find("Effects \xc2\xb7 ") != std::string::npos) return 1;   // utf8-ok: data
        return 2;
    };
    std::sort(cats.begin(), cats.end(), [&](const std::string& a, const std::string& b) {
        const auto fa = splitCategory(a).first, fb = splitCategory(b).first;
        if (fa != fb) return fa < fb;
        return rank(a) != rank(b) ? rank(a) < rank(b) : a < b;
    });

    CategoryGroups out;
    out.reserve(cats.size());
    for (const auto& cat : cats) {
        auto& v = byCat[cat];
        std::sort(v.begin(), v.end());
        out.emplace_back(cat, std::move(v));
    }
    return out;
}

std::vector<std::pair<std::string, CategoryGroups>>
groupByOrigin(const std::vector<std::string>& classNames) {
    std::unordered_map<std::string, std::vector<std::string>> byOrigin;
    for (const auto& c : classNames) {
        if (categoryOf(c).empty()) continue;
        byOrigin[originOf(c)].push_back(c);
    }
    std::vector<std::string> order;
    for (const auto& kv : byOrigin) order.push_back(kv.first);
    std::sort(order.begin(), order.end());
    if (auto p = std::find(order.begin(), order.end(), "Plugins"); p != order.end()) {
        order.erase(p);
        order.push_back("Plugins");
    }

    std::vector<std::pair<std::string, CategoryGroups>> out;
    for (const auto& origin : order) {
        auto it = byOrigin.find(origin);
        if (it == byOrigin.end() || it->second.empty()) continue;
        out.emplace_back(origin, groupByCategory(it->second));
    }
    return out;
}

}
