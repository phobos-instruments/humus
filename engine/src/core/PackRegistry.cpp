#include "core/PackRegistry.h"

#include "core/BuiltinPacks.h"

#include <juce_core/juce_core.h>

#ifndef HUM_PACKS_DIR
#define HUM_PACKS_DIR ""
#endif

namespace hum {

PackRegistry& PackRegistry::instance() {
    static PackRegistry r;
    return r;
}

std::string PackRegistry::packsRootDir() {
    auto holdsPacks = [](const juce::File& d) {
        if (!d.isDirectory()) return false;
        for (const auto& c : d.findChildFiles(juce::File::findDirectories, false))
            if (c.getChildFile("pack.json").existsAsFile()) return true;
        return false;
    };
    const auto exeDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                            .getParentDirectory();
    for (const auto& c : {exeDir.getParentDirectory().getChildFile("Resources").getChildFile("packs"),
                          exeDir.getChildFile("packs"),
                          exeDir.getParentDirectory().getChildFile("packs")})
        if (holdsPacks(c)) return c.getFullPathName().toStdString();
    juce::File repo(juce::String(HUM_PACKS_DIR));
    if (holdsPacks(repo)) return repo.getFullPathName().toStdString();
    return {};
}

void PackRegistry::loadBuiltinPacks() {
    if (loaded_) return;
    loaded_ = true;
    const auto root = packsRootDir();
    if (root.empty()) return;
    for (const auto& pack : builtinPacks()) {
        const auto packDir = juce::File(juce::String(root)).getChildFile(pack.id);
        if (packDir.getChildFile("pack.json").existsAsFile())
            loadPackDir(packDir.getFullPathName().toStdString(), true);
    }
}

bool PackRegistry::loadPackDir(const std::string& dir, bool builtin) {
    juce::File d{juce::String(dir)};
    PackManifest pm;
    if (!parsePackManifest(d.getChildFile("pack.json").loadFileAsString().toStdString(), pm))
        return false;
    for (auto& existing : packs_)
        if (existing.manifest.id == pm.id) return false;

    Pack pack;
    pack.manifest = std::move(pm);
    pack.dir = dir;
    pack.builtin = builtin;

    const auto organismsDir = d.getChildFile("organisms");
    for (const auto& cdir : organismsDir.findChildFiles(juce::File::findDirectories, false)) {
        const auto mf = cdir.getChildFile("organism.json");
        if (!mf.existsAsFile()) continue;
        OrganismManifest cm;
        if (!parseOrganismManifest(mf.loadFileAsString().toStdString(), cm)) continue;
        cm.dir = cdir.getFullPathName().toStdString();
        for (auto& c : cm.classes)
            for (auto& pd : loadPresetTree(cdir.getChildFile("presets"), c.className))
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
