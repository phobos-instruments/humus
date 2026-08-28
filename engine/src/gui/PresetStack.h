#pragma once
#include <string>
#include <vector>

#include "core/ParamSchema.h"
#include "gui/PresetLibrary.h"
#include "io/PatchDocument.h"

namespace hum::presets {

enum class Source { Shipped, Library, Patch };

struct Ref {
    Source source = Source::Patch;
    int number = 0;
    std::string name;
    bool valid() const { return source == Source::Patch ? number > 0 : !name.empty(); }
    bool operator==(const Ref& o) const {
        return source == o.source && (source == Source::Patch ? number == o.number
                                                              : name == o.name);
    }
};

struct Entry {
    Ref ref;
    std::string name;
    std::string group;
    std::vector<Parameter> properties;
};

inline std::vector<Entry> stack(const OrganismModel* c) {
    std::vector<Entry> out;
    if (c == nullptr) return out;
    const auto& schema = schemaFor(c->classRaw);

    auto add = [&](Source src, const PresetDef& def) {
        Ref r; r.source = src; r.name = def.name;
        auto pm = presetlib::toModel(def, schema, 0);
        out.push_back({r, def.name, def.group, std::move(pm.properties)});
    };
    for (const auto& def : factoryPresetsFor(c->classRaw)) add(Source::Shipped, def);
    if (juce::JUCEApplicationBase::getInstance() != nullptr)
        for (const auto& def : presetlib::list(c->classRaw)) add(Source::Library, def);
    for (const auto& pm : c->presets) {
        Ref r; r.source = Source::Patch; r.number = pm.number; r.name = pm.name;
        out.push_back({r, pm.name, {}, pm.properties});
    }

    return out;
}

inline const Entry* find(const std::vector<Entry>& all, const Ref& ref) {
    for (const auto& e : all) if (e.ref == ref) return &e;
    return nullptr;
}

inline Ref adjacentTo(const std::vector<Entry>& all, const Ref& cur, int dir) {
    if (all.empty()) return {};
    int idx = -1;
    for (size_t i = 0; i < all.size(); ++i) if (all[i].ref == cur) idx = (int) i;
    const int n = (int) all.size();
    const int next = idx < 0 ? (dir > 0 ? 0 : n - 1) : ((idx + dir) % n + n) % n;
    return all[(size_t) next].ref;
}

}
