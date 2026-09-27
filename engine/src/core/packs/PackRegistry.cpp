// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/packs/PackRegistry.h"

#include <filesystem>
#include <system_error>

#include "core/json/Json.h"
#include "hum/FileBytes.h"

namespace hum {

PackRegistry& PackRegistry::instance() {
    static PackRegistry r;
    return r;
}

const PackRoots* PackRegistry::setRoots(const PackRoots* roots) {
    const auto* previous = roots_;
    roots_ = roots;
    return previous;
}

std::string PackRegistry::builtinRoot() const { return roots_ != nullptr ? roots_->builtinRoot() : std::string(); }

void PackRegistry::loadBuiltinPacks() {
    if (loaded_ || roots_ == nullptr) return;
    loaded_ = true;
    const auto root = roots_->builtinRoot();
    if (root.empty()) return;
    std::error_code ec;
    for (const auto& id : roots_->builtinIds()) {
        const auto dir = utf8Path(root) / utf8Path(id);
        if (std::filesystem::is_regular_file(dir / "pack.json", ec)) loadPackDir(utf8Text(dir), true);
    }
}

bool PackRegistry::loadPackDir(const std::string& dir, bool builtin) {
    namespace fs = std::filesystem;
    std::string text;
    json::readTextFile(utf8Text(utf8Path(dir) / "pack.json"), text);
    PackManifest pm;
    if (!parsePackManifest(text, pm)) return false;
    for (auto& existing : packs_)
        if (existing.manifest.id == pm.id) return false;

    Pack pack;
    pack.manifest = std::move(pm);
    pack.dir = dir;
    pack.builtin = builtin;

    std::error_code ec;
    for (fs::directory_iterator it(utf8Path(dir) / "organisms", ec), end; !ec && it != end; it.increment(ec)) {
        if (!it->is_directory(ec)) continue;
        const auto cdir = it->path();
        std::string manifest;
        if (!fs::is_regular_file(cdir / "organism.json", ec) || !json::readTextFile(utf8Text(cdir / "organism.json"), manifest))
            continue;
        OrganismManifest cm;
        if (!parseOrganismManifest(manifest, cm)) continue;
        cm.dir = utf8Text(cdir);
        for (auto& c : cm.classes)
            for (auto& pd : loadPresetTree(utf8Text(cdir / "presets"), c.className))
                c.presets.push_back(std::move(pd));
        pack.organisms.push_back(std::move(cm));
    }

    if (pack.organisms.empty()) return false;

    packs_.push_back(std::move(pack));
    indexClasses((int) packs_.size() - 1);
    return true;
}

void PackRegistry::indexClasses(int packIdx) {
    const auto& organisms = packs_[(size_t) packIdx].organisms;
    for (int f = 0; f < (int) organisms.size(); ++f)
        for (int c = 0; c < (int) organisms[(size_t) f].classes.size(); ++c)
            byClass_.emplace(organisms[(size_t) f].classes[(size_t) c].className,
                             Ref{packIdx, f, c});
}

void PackRegistry::removePack(const std::string& id) {
    for (int i = 0; i < (int) packs_.size(); ++i) {
        if (packs_[(size_t) i].manifest.id != id) continue;
        packs_.erase(packs_.begin() + i);
        byClass_.clear();
        for (int p = 0; p < (int) packs_.size(); ++p) indexClasses(p);
        return;
    }
}

PackRegistry::Pack* PackRegistry::packById(const std::string& id) {
    for (auto& p : packs_)
        if (p.manifest.id == id) return &p;
    return nullptr;
}

const OrganismClassManifest* PackRegistry::classManifest(const std::string& className) const {
    auto it = byClass_.find(className);
    if (it == byClass_.end()) return nullptr;
    const auto& r = it->second;
    return &packs_[(size_t) r.pack].organisms[(size_t) r.folder].classes[(size_t) r.cls];
}

const PackRegistry::Pack* PackRegistry::packOf(const std::string& className) const {
    auto it = byClass_.find(className);
    return it == byClass_.end() ? nullptr : &packs_[(size_t) it->second.pack];
}

const OrganismManifest* PackRegistry::folderOf(const std::string& className) const {
    auto it = byClass_.find(className);
    if (it == byClass_.end()) return nullptr;
    const auto& r = it->second;
    return &packs_[(size_t) r.pack].organisms[(size_t) r.folder];
}

bool PackRegistry::isClassEnabled(const std::string& className) const {
    const auto* p = packOf(className);
    return p ? p->enabled : true;
}

void PackRegistry::setPackEnabled(const std::string& id, bool enabled) {
    if (auto* p = packById(id)) p->enabled = enabled;
}

}
