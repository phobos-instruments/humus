// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <functional>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "hum/Organism.h"

namespace hum {

class Registry {
public:
    using Factory = std::function<OrganismPtr()>;
    using PatternFactory = std::function<OrganismPtr(const std::string&)>;
    using LayoutProvider =
        std::function<std::string(const std::string& genId, const std::string& className)>;

    static Registry& instance();

    void registerClass(const std::string& className, Factory f) {
        hostRegistry() = this;
        factories_[className] = std::move(f);
    }
    void registerPatternFallback(std::string packId, PatternFactory f) {
        patterns_.push_back({std::move(packId), std::move(f)});
    }
    void registerLayoutProvider(std::string packId, LayoutProvider f) {
        layouts_.push_back({std::move(packId), std::move(f)});
    }

    std::string layoutJson(const std::string& genId, const std::string& className) const;

    void unregisterClass(const std::string& className) {
        factories_.erase(className);
    }

    bool isKnown(const std::string& className) const {
        return factories_.find(className) != factories_.end();
    }

    void registerFlag(std::string name, std::function<bool()> f) {
        flags_[std::move(name)] = std::move(f);
    }
    bool flag(const std::string& name) const {
        const auto it = flags_.find(name);
        return it != flags_.end() && it->second();
    }

    static Registry*& hostRegistry() {
        static Registry* r = nullptr;
        return r;
    }

    void registerPathResolver(std::function<std::string(const std::string&,
                                                        const std::string&)> f) {
        resolvePath_ = std::move(f);
    }
    std::string resolvePath(const std::string& ref, const std::string& className) const {
        return resolvePath_ ? resolvePath_(ref, className) : ref;
    }

    std::vector<std::string> classNames() const;

    OrganismPtr create(const std::string& className) const;

    OrganismPtr createExact(const std::string& className) const {
        auto it = factories_.find(className);
        if (it != factories_.end()) return it->second();
        for (const auto& p : patterns_)
            if (auto c = p.second(className)) return c;
        return nullptr;
    }

    std::string layoutJsonExact(const std::string& genId, const std::string& className) const {
        for (const auto& p : layouts_)
            if (auto json = p.second(genId, className); !json.empty()) return json;
        return {};
    }

private:
    std::unordered_map<std::string, Factory> factories_;
    std::vector<std::pair<std::string, PatternFactory>> patterns_;
    std::map<std::string, std::function<bool()>> flags_;
    std::function<std::string(const std::string&, const std::string&)> resolvePath_;
    std::vector<std::pair<std::string, LayoutProvider>> layouts_;
};

void registerBuiltinOrganisms();

inline std::string resolvePath(const std::string& ref, const std::string& className) {
    auto* r = Registry::hostRegistry();
    return r != nullptr ? r->resolvePath(ref, className) : ref;
}

}
