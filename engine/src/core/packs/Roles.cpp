// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/packs/Roles.h"

#include <algorithm>

#include "core/packs/ClassString.h"
#include "core/packs/PackRegistry.h"

namespace hum {

const std::vector<RoleContract>& roleContracts() {
    static const std::vector<RoleContract> contracts{
        {role::kAudioIn, false, {}},
        {role::kAudioOut, false, {}},
        {role::kMasterIn, true, {}},
        {role::kMasterOut, true, {}},
        {role::kMidiIn, false, {"Port"}},
        {role::kMidiOut, false, {"Port", "Channel", "Controller", "Value", "Note", "Velocity", "Gate"}},
        {role::kAudioTrack, true, {"Record"}},
        {role::kVideoTrack, true, {}},
        {role::kMidiTrack, true, {"Target"}},
        {role::kTuning, true, {"Preset", "Divisions", "RootHz", "Plugins"}},
        {role::kSampleKit, true, {"Mode", "File1", "Root1"}},
        {role::kDefaultInstrument, true, {}},
        {role::kMixAnchor, false, {}},
        {role::kDeck, false, {"BPM", "GridOffset", "Key"}},
        {role::kNoteLanes, false, {"Note_1"}},
        {role::kClipPads, false, {"File1", "In1", "Out1"}},
    };
    return contracts;
}

namespace {
bool declares(const OrganismClassManifest& m, std::string_view role) {
    return std::find(m.roles.begin(), m.roles.end(), role) != m.roles.end();
}
}

bool classHasRole(const std::string& classString, std::string_view role) {
    auto& registry = PackRegistry::instance();
    registry.loadBuiltinPacks();
    const auto* m = registry.classManifest(parseClassString(classString).display);
    return m != nullptr && declares(*m, role);
}

std::string classWithRole(std::string_view role) {
    auto& registry = PackRegistry::instance();
    registry.loadBuiltinPacks();
    for (const auto& pack : registry.packs())
        for (const auto& folder : pack.organisms)
            for (const auto& cls : folder.classes)
                if (declares(cls, role) && registry.isClassEnabled(cls.className)) return cls.className;
    return {};
}

}
