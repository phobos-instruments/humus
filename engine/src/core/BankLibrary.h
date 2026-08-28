#pragma once
#include <string>
#include <vector>

#include <juce_core/juce_core.h>

#include "core/Catalogue.h"

namespace hum::banks {

inline constexpr const char* kLegacyPrefix = "bank:";

struct Slot {
    std::string kind;
    std::string filter;
    std::string factory;
};

inline const catalogue::Kind& kindFor(const Slot& slot) {
    const auto* k = catalogue::find(slot.kind);
    return k != nullptr ? *k : *catalogue::find("Samples");
}

struct FactoryEntry { std::string ref, name, kind; };

inline std::vector<FactoryEntry> factoryEntries(const Slot& slot) {
    std::vector<FactoryEntry> out;
    for (const auto& row : juce::StringArray::fromTokens(
             juce::String(juce::CharPointer_UTF8(slot.factory.c_str())), ";", "")) {
        const auto f = juce::StringArray::fromTokens(row.trim(), "|", "");
        if (f.size() == 3)
            out.push_back({f[0].toStdString(), f[1].toStdString(), f[2].toStdString()});
    }
    return out;
}

inline std::string kindOf(const juce::File& f, const Slot& slot) {
    const auto& kind = kindFor(slot);
    const auto folder = f.getParentDirectory().getFileName();
    if (folder.isNotEmpty() && folder != juce::String(kind.id) && folder != "banks")
        return folder.toUpperCase().toStdString();
    return catalogue::badgeFor(f, kind);
}

inline bool browsable(const juce::File& f, const Slot& slot) {
    const auto& kind = kindFor(slot);
    return catalogue::matches(f, slot.filter.empty() ? kind.wildcard : slot.filter);
}

inline std::vector<juce::File> roots(const Slot& slot, const std::string& className = {}) {
    return catalogue::roots(kindFor(slot), className);
}

inline std::vector<juce::File> scan(const Slot& slot, const std::string& className = {}) {
    std::vector<juce::File> out;
    for (const auto& e : catalogue::scan(kindFor(slot), className)) out.push_back(e.file);
    return out;
}

inline std::string referenceFor(const juce::File& f, const Slot& slot,
                                const std::string& className = {}) {
    return catalogue::refFor(f, kindFor(slot), className);
}

inline std::string resolve(const std::string& ref, const std::string& className) {
    if (ref.rfind(kLegacyPrefix, 0) != 0) return catalogue::resolve(ref, className);
    const auto leaf = ref.substr(std::string(kLegacyPrefix).size());
    for (const auto& kind : catalogue::kinds()) {
        const auto asKind = std::string(kAssetScheme) + kind.id + "/" + leaf;
        if (const auto hit = catalogue::resolve(asKind, className); hit != asKind) return hit;
    }
    return ref;
}

inline std::vector<std::string> rollable(const Slot& slot, const std::string& className) {
    std::vector<std::string> out;
    for (const auto& e : factoryEntries(slot)) out.push_back(e.ref);
    for (const auto& f : scan(slot, className))
        if (browsable(f, slot)) out.push_back(referenceFor(f, slot, className));
    return out;
}

}
