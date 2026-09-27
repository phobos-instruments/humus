// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/browser/Loaders.h"

#include <algorithm>

#include <juce_core/juce_core.h>

#include "core/browser/BrowserPlaces.h"
#include "core/browser/FileKind.h"
#include "core/library/BankLibrary.h"
#include "core/packs/PackRegistry.h"
#include "gui/editor/BankSlotSpec.h"
#include "gui/editor/juce/JuceMediaLibrary.h"
#include "gui/host/PatcherHost.h"

namespace hum::browser {

namespace {

std::string patternsOf(const LayoutSpec::Control& c) {
    using CT = LayoutSpec::ControlType;
    if (const auto filter = c.extraOr("filter"); !filter.empty()) return filter;
    switch (c.type) {
        case CT::SoundFile: case CT::FileBox: return juceAudioPatterns();
        case CT::BankFile: return banks::kindFor(banks::Slot{c.extraOr("kind", "Samples"), {}, {}}).wildcard;
        case CT::ScaleFile: return "*.scl";
        default: return {};
    }
}

bool loadsFiles(const LayoutSpec::Control& c) {
    using CT = LayoutSpec::ControlType;
    const bool slot = c.type == CT::SoundFile || c.type == CT::FileBox || c.type == CT::BankFile || c.type == CT::ScaleFile;
    return slot && !c.param.empty() && c.extraOr("pick") != "save";
}

int categoryRank(const std::string& category, Kind kind) {
    static const char* const kSounds[] = {"Instruments", "Players", "Sequencers", "Spectral", "Effects", "Visual"};
    static const char* const kImpulses[] = {"Effects", "Instruments", "Players", "Spectral"};
    const auto* order = kind == Kind::Impulse ? kImpulses : kSounds;
    const int n = kind == Kind::Impulse ? 4 : 6;
    for (int i = 0; i < n; ++i)
        if (category == order[i]) return i;
    return category == "Input/Output" ? n + 2 : n + 1;
}

int rankOf(const Loader& l, Kind kind) {
    constexpr int kKindMiss = 100;
    const char* shelf = shelfFor(kind);
    const bool made = shelf != nullptr && l.slotKind == shelf;
    return (made ? 0 : kKindMiss) + categoryRank(l.category, kind);
}

bool matches(const std::string& path, const std::string& patterns) {
    const auto name = juce::String::fromUTF8(fileName(path).c_str());
    for (const auto& one : juce::StringArray::fromTokens(juce::String::fromUTF8(patterns.c_str()), ";", ""))
        if (name.matchesWildcard(one.trim(), true)) return true;
    return false;
}

}

const LoaderTable& LoaderTable::shared() {
    static const LoaderTable table = buildLoaderTable();
    return table;
}

LoaderTable buildLoaderTable() {
    LoaderTable table;
    for (const auto& pack : PackRegistry::instance().packs()) {
        if (!pack.enabled) continue;
        for (const auto& folder : pack.organisms)
            for (const auto& cls : folder.classes) {
                const auto layoutClass = cls.canonical.empty() ? cls.className : cls.canonical;
                for (const auto& c : layoutSpecFor(layoutClass).controls)
                    if (loadsFiles(c))
                        if (const auto patterns = patternsOf(c); !patterns.empty())
                            table.add({cls.className, c.param, patterns, cls.hidden, cls.category, c.extraOr("kind")});
            }
    }
    return table;
}

std::vector<Loader> LoaderTable::loadersFor(const std::string& path) const {
    std::vector<Loader> out;
    const auto kind = kindOfFile(path);
    for (const auto& l : loaders_) {
        if (l.hidden || !matches(path, l.patterns)) continue;
        const auto it = std::find_if(out.begin(), out.end(), [&](const Loader& o) { return o.className == l.className; });
        if (it == out.end()) out.push_back(l);
        else if (rankOf(l, kind) < rankOf(*it, kind)) *it = l;
    }
    std::stable_sort(out.begin(), out.end(), [kind](const Loader& a, const Loader& b) {
        return rankOf(a, kind) < rankOf(b, kind);
    });
    return out;
}

std::vector<std::string> LoaderTable::classesFor(const std::string& path) const {
    std::vector<std::string> out;
    for (const auto& l : loadersFor(path)) out.push_back(l.className);
    return out;
}

const Loader* LoaderTable::slotIn(const std::string& className, const std::string& path) const {
    for (const auto& l : loaders_)
        if (l.className == className && matches(path, l.patterns)) return &l;
    return nullptr;
}

std::string loadInto(PatcherHost& host, const std::string& node, const std::string& path) {
    const auto* cm = host.model().byName(node);
    if (cm == nullptr) return {};
    for (const auto* cls : {&cm->displayClass, &cm->classRaw})
        if (const auto* slot = LoaderTable::shared().slotIn(*cls, path)) {
            host.setParamText(node, slot->param, path);
            return slot->param;
        }
    return {};
}

std::vector<std::string> loadIntoNew(PatcherHost& host, const std::string& className, const std::vector<std::string>& paths,
                                     int x, int y, const std::string& scope) {
    constexpr int kStackStep = 48;
    std::vector<std::string> made;
    host.beginTransaction();
    for (const auto& path : paths) {
        const auto* slot = LoaderTable::shared().slotIn(className, path);
        if (slot == nullptr) continue;
        const auto name = host.addOrganism(className, {x, y + kStackStep * (int) made.size()}, scope);
        if (name.empty()) continue;
        host.setParamText(name, slot->param, path);
        made.push_back(name);
    }
    host.endTransaction();
    return made;
}

std::string loadsIntoText(const std::vector<std::string>& classes, int most) {
    std::string out;
    for (int i = 0; i < (int) classes.size() && i < most; ++i) out += (i > 0 ? ", " : "") + classes[(size_t) i];
    if ((int) classes.size() > most) out += " +" + std::to_string(classes.size() - (size_t) most);
    return out;
}

}
