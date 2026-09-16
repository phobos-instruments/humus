// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHostPresets.h"
#include "gui/properties/PresetLibrary.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "core/packs/PackRegistry.h"

namespace hum {

using presets::Ref;
using presets::Source;

namespace {
PresetModel* findPreset(OrganismModel& c, int number) {
    for (auto& p : c.presets) if (p.number == number) return &p;
    return nullptr;
}
int firstFreeSlot(const OrganismModel& c) {
    for (int n = 1;; ++n) {
        bool taken = false;
        for (auto& p : c.presets) if (p.number == n) { taken = true; break; }
        if (!taken) return n;
    }
}
PresetModel* findByName(OrganismModel& c, const std::string& name) {
    if (name.empty()) return nullptr;
    for (auto& p : c.presets) if (p.name == name) return &p;
    return nullptr;
}
}

Ref PresetHost::current(const std::string& name) const {
    Ref r;
    const auto* c = doc_.document().byName(name);
    if (c == nullptr) return r;
    if (!c->currentPresetName.empty()) {
        r.source = c->currentPresetSource == "library" ? Source::Library : Source::Shipped;
        r.name = c->currentPresetName;
        return r;
    }
    r.source = Source::Patch;
    r.number = c->currentPreset;
    if (const auto* pm = findPreset(const_cast<OrganismModel&>(*c), r.number)) r.name = pm->name;
    return r;
}

void PresetHost::setCurrent(const std::string& name, const Ref& ref) {
    auto* c = doc_.document().byName(name);
    if (c == nullptr) return;
    if (ref.source == Source::Patch) {
        c->currentPreset = ref.number;
        c->currentPresetName.clear();
        c->currentPresetSource.clear();
    } else {
        c->currentPreset = 0;
        c->currentPresetName = ref.name;
        c->currentPresetSource = ref.source == Source::Library ? "library" : "shipped";
    }
}

int PresetHost::store(const std::string& name, const Ref& ref) {
    auto* c = doc_.document().byName(name);
    if (!c) return 0;
    host_.pushUndo();
    PresetModel* pm = ref.source == Source::Patch && ref.number > 0
                          ? findPreset(*c, ref.number)
                          : findByName(*c, ref.name);
    if (!pm) {
        PresetModel np;
        np.number = firstFreeSlot(*c);
        np.name = ref.name;
        c->presets.push_back(std::move(np));
        std::sort(c->presets.begin(), c->presets.end(),
                  [](const PresetModel& a, const PresetModel& b) { return a.number < b.number; });
        pm = findByName(*c, ref.name);
        if (!pm) pm = &c->presets.back();
    }
    pm->properties = settingsOnly(c->properties);
    setCurrent(name, {Source::Patch, pm->number, pm->name});
    c->presetDirty = false;
    return pm->number;
}

void PresetHost::recall(const std::string& name, const Ref& ref) {
    auto* c = doc_.document().byName(name);
    if (!c) return;

    std::string node = name;
    bool reshaped = false;
    if (ref.source == Source::Shipped) {
        for (const auto& def : factoryPresetsFor(c->classRaw)) {
            if (def.name != ref.name) continue;
            for (const auto& [k, v] : def.values) {
                if (k != "Bands") continue;
                const std::string cls = c->displayClass;
                const auto a = cls.find_first_of("0123456789");
                if (a == std::string::npos) break;
                const auto b = cls.find_first_not_of("0123456789", a);
                const std::string next = cls.substr(0, a)
                    + std::to_string((int) std::llround(v))
                    + (b == std::string::npos ? std::string() : cls.substr(b));
                if (next != cls && PackRegistry::instance().classManifest(next) != nullptr) {
                    node = nodes_.replaceOrganism(name, next);
                    reshaped = true;
                }
                break;
            }
            break;
        }
    }

    auto* cn = doc_.document().byName(node);
    if (!cn) return;
    const auto all = presets::stack(cn);
    const auto* e = presets::find(all, ref);
    if (e == nullptr) return;
    if (!reshaped) host_.pushParamStep();
    for (auto& pr : e->properties) {
        if (pr.name == "Bands") continue;
        if (pr.type == "text" || pr.type == "soundfile" || pr.type == "rhythmic-unit")
            host_.setParamText(node, pr.name, pr.text);
        else
            host_.setParam(node, pr.name, pr.value);
    }
    setCurrent(node, ref);
    cn->presetDirty = false;
    if (reshaped) doc_.noteTopologyChanged();
}

void PresetHost::clear(const std::string& name, const Ref& ref) {
    auto* c = doc_.document().byName(name);
    if (!c) return;
    if (ref.source == Source::Shipped) return;
    if (ref.source == Source::Library) {
        presetlib::remove(c->classRaw, ref.name);
        if (current(name) == ref) setCurrent(name, {});
        return;
    }
    if (!findPreset(*c, ref.number)) return;
    host_.pushUndo();
    auto& v = c->presets;
    v.erase(std::remove_if(v.begin(), v.end(),
                           [&](const PresetModel& p) { return p.number == ref.number; }), v.end());
    if (c->currentPreset == ref.number) setCurrent(name, {});
}

void PresetHost::rename(const std::string& name, const Ref& ref, const std::string& newName) {
    auto* c = doc_.document().byName(name);
    if (!c) return;
    if (ref.source == Source::Library) {
        for (auto def : presetlib::list(c->classRaw))
            if (def.name == ref.name) {
                presetlib::remove(c->classRaw, ref.name);
                def.name = newName;
                presetlib::save(c->classRaw, def);
                setCurrent(name, {Source::Library, 0, newName});
                return;
            }
        return;
    }
    if (ref.source == Source::Shipped) {
        store(name, {Source::Patch, 0, newName});
        return;
    }
    if (auto* pm = findPreset(*c, ref.number)) { host_.pushUndo(); pm->name = newName; }
}

Ref PresetHost::recallAdjacent(const std::string& name, int dir) {
    const auto all = presets::stack(doc_.document().byName(name));
    if (all.empty()) return {};
    const Ref next = presets::adjacentTo(all, current(name), dir);
    recall(name, next);
    return next;
}

int PresetHost::adopt(const std::string& name, PresetModel pm) {
    auto* c = doc_.document().byName(name);
    if (!c) return 0;
    host_.pushUndo();
    pm.number = firstFreeSlot(*c);
    const int n = pm.number;
    c->presets.push_back(std::move(pm));
    std::sort(c->presets.begin(), c->presets.end(),
              [](const PresetModel& a, const PresetModel& b) { return a.number < b.number; });
    return n;
}

PresetModel PresetHost::presetClip_;
std::string PresetHost::presetClipClass_;
bool PresetHost::hasPresetClip_ = false;

void PresetHost::copy(const std::string& name, const Ref& ref) {
    auto* c = doc_.document().byName(name);
    if (!c) return;
    const auto all = presets::stack(doc_.document().byName(name));
    const auto* e = presets::find(all, ref);
    if (e == nullptr) return;
    presetClip_ = PresetModel{};
    presetClip_.name = e->name;
    presetClip_.properties = e->properties;
    presetClipClass_ = c->displayClass;
    hasPresetClip_ = true;
}

void PresetHost::cut(const std::string& name, const Ref& ref) {
    copy(name, ref);
    if (hasPresetClip_) clear(name, ref);
}

bool PresetHost::canPaste(const std::string& name) const {
    auto* c = doc_.document().byName(name);
    return hasPresetClip_ && c && c->displayClass == presetClipClass_;
}

bool PresetHost::paste(const std::string& name) {
    if (!canPaste(name)) return false;
    auto* c = doc_.document().byName(name);
    host_.pushUndo();
    PresetModel pm = presetClip_;
    pm.number = firstFreeSlot(*c);
    c->presets.push_back(pm);
    std::sort(c->presets.begin(), c->presets.end(),
              [](const PresetModel& a, const PresetModel& b) { return a.number < b.number; });
    setCurrent(name, {Source::Patch, pm.number, pm.name});
    return true;
}

}
