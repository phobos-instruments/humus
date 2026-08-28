#include "core/PackUpdates.h"

#include <juce_core/juce_core.h>

namespace hum {

namespace {
int component(const juce::String& s) {
    return s.containsOnly("0123456789") && s.isNotEmpty() ? s.getIntValue() : 0;
}
}

int compareVersions(const std::string& a, const std::string& b) {
    juce::StringArray pa, pb;
    pa.addTokens(juce::String(a), ".", "");
    pb.addTokens(juce::String(b), ".", "");
    const int n = juce::jmax(pa.size(), pb.size());
    for (int i = 0; i < n; ++i) {
        const int x = i < pa.size() ? component(pa[i]) : 0;
        const int y = i < pb.size() ? component(pb[i]) : 0;
        if (x != y) return x < y ? -1 : 1;
    }
    return 0;
}

std::vector<PackUpdate> availableUpdates(const std::string& manifestJson,
                                         const std::vector<InstalledPack>& installed,
                                         int hostAbi,
                                         const std::string& platformTag) {
    std::vector<PackUpdate> out;
    const auto root = juce::JSON::parse(juce::String(manifestJson));
    if (!root.isObject()) return out;
    auto* packs = root.getProperty("packs", {}).getArray();
    if (packs == nullptr) return out;

    for (const auto& p : *packs) {
        PackUpdate u;
        u.id = p.getProperty("id", {}).toString().toStdString();
        u.version = p.getProperty("version", {}).toString().toStdString();
        if (u.id.empty() || u.version.empty()) continue;

        if ((int) p.getProperty("abi", -1) != hostAbi) continue;
        auto* plat = p.getProperty("platforms", {}).getDynamicObject();
        if (plat == nullptr) continue;
        const auto url = plat->getProperty(juce::String(platformTag)).toString();
        if (url.isEmpty()) continue;
        u.url = url.toStdString();
        u.notes = p.getProperty("notes", {}).toString().toStdString();

        for (const auto& ins : installed)
            if (ins.id == u.id && compareVersions(ins.version, u.version) < 0) {
                out.push_back(u);
                break;
            }
    }
    return out;
}

}
