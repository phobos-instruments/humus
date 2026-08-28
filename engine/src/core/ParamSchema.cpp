#include "core/ParamSchema.h"

#include "core/ClassString.h"
#include "core/PackRegistry.h"
#include "core/PluginHost.h"

namespace hum {

const std::vector<ParamDesc>& schemaFor(const std::string& className) {
    static const std::vector<ParamDesc> empty;
    auto& packs = PackRegistry::instance();
    packs.loadBuiltinPacks();
    if (const auto* m = packs.classManifest(className)) return m->params;
    if (isPluginKind(parseClassString(className).kind))
        return PluginHost::instance().schemaFor(className);
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
