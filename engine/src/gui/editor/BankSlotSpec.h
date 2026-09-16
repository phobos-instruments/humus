// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include "core/library/BankLibrary.h"
#include "core/packs/PackRegistry.h"
#include "hum/LayoutSpec.h"
#include "hum/Registry.h"
#include "gui/editor/LayoutLoader.h"

namespace hum {

inline LayoutSpec makeGeneratedLayout(const std::string& id, const std::string& cls) {
    const auto json = Registry::instance().layoutJson(id, cls);
    return json.empty() ? LayoutSpec{} : loadLayoutSpec(json);
}

inline LayoutSpec layoutSpecFor(const std::string& cls) {
    if (const auto* m = PackRegistry::instance().classManifest(cls)) {
        const auto& e = m->editor;
        if (e.rfind("gen:", 0) == 0) return makeGeneratedLayout(e.substr(4), cls);
        if (e.rfind("layout:", 0) == 0)
            if (const auto* folder = PackRegistry::instance().folderOf(cls))
                return loadLayoutSpecFromFile(folder->dir + "/" + e.substr(7));
        return {};
    }
    return makeGeneratedLayout("", cls);
}

inline banks::Slot bankSlotFor(const std::string& cls, const std::string& param = {}) {
    for (const auto& c : layoutSpecFor(cls).controls) {
        if (c.type != LayoutSpec::ControlType::BankFile
            && c.type != LayoutSpec::ControlType::SoundFile) continue;
        if (!param.empty() && c.param != param) continue;
        return {c.extraOr("kind", "Samples"), c.extraOr("filter"), c.extraOr("factory")};
    }
    return {"Samples", {}, {}};
}

}
