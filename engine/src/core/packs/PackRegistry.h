// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "core/packs/PackManifest.h"

namespace hum {

class PackRegistry {
public:
    static PackRegistry& instance();

    struct Pack {
        PackManifest manifest;
        std::string dir;
        bool builtin = true;
        bool enabled = true;
        std::vector<OrganismManifest> organisms;
    };

    void loadBuiltinPacks();

    const std::vector<Pack>& packs() const { return packs_; }
    Pack* packById(const std::string& id);

    const OrganismClassManifest* classManifest(const std::string& className) const;
    const Pack* packOf(const std::string& className) const;
    const OrganismManifest* folderOf(const std::string& className) const;

    bool isClassEnabled(const std::string& className) const;
    void setPackEnabled(const std::string& id, bool enabled);

    static std::string packsRootDir();

    bool loadPackDir(const std::string& dir, bool builtin);
    void removePack(const std::string& id);

private:
    void indexClasses(int packIdx);

    std::vector<Pack> packs_;
    struct Ref { int pack; int folder; int cls; };
    std::unordered_map<std::string, Ref> byClass_;
    bool loaded_ = false;
};

}
