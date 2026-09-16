// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/params/ParamSchema.h"

#include "core/packs/ClassString.h"
#include "core/packs/PackRegistry.h"
#include "core/graph/PodModel.h"
#include "core/plugins/PluginHost.h"

namespace hum {

const std::vector<ParamDesc>& schemaFor(const std::string& className) {
    static const std::vector<ParamDesc> empty;
    auto& packs = PackRegistry::instance();
    packs.loadBuiltinPacks();
    if (const auto* m = packs.classManifest(className)) return m->params;
    if (isPluginKind(parseClassString(className).kind))
        return PluginHost::instance().schemaFor(className);
    if (pods::isControlPortClass(className)) {
        static const std::vector<ParamDesc> controlPort = [] {
            ParamDesc d;
            d.name = pods::kControlPortParam;
            d.min = 0.0;
            d.max = 1.0;
            d.def = 0.0;
            d.socket = true;
            return std::vector<ParamDesc>{d};
        }();
        return controlPort;
    }
    return empty;
}

const std::vector<PresetDef>& factoryPresetsFor(const std::string& className) {
    static const std::vector<PresetDef> empty;
    auto& packs = PackRegistry::instance();
    packs.loadBuiltinPacks();
    if (const auto* m = packs.classManifest(className)) return m->presets;
    return empty;
}

}
