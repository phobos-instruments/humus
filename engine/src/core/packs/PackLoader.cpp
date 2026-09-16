// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/packs/PackLoader.h"

#include "core/app/AppPaths.h"
#include "core/packs/PackRegistry.h"
#include "hum/AbiShape.h"
#include "hum/PackEntry.h"
#include "hum/Registry.h"

namespace hum {

PackLoader& PackLoader::instance() {
    static PackLoader l;
    return l;
}

namespace {
juce::File& userPacksOverride() {
    static juce::File f;
    return f;
}
}

juce::File PackLoader::userPacksDir() {
    if (userPacksOverride() != juce::File{}) return userPacksOverride();
    return appDataDir().getChildFile("packs");
}

void PackLoader::setUserPacksDirForTesting(const juce::File& dir) { userPacksOverride() = dir; }

std::string PackLoader::platformTag() {
#if defined(__linux__) && defined(__x86_64__)
    return "linux-x86_64";
#elif defined(__linux__) && defined(__aarch64__)
    return "linux-aarch64";
#elif defined(__APPLE__) && defined(__aarch64__)
    return "darwin-arm64";
#elif defined(__APPLE__)
    return "darwin-x86_64";
#elif defined(_WIN64)
    return "windows-amd64";
#else
    return "unknown";
#endif
}

void PackLoader::loadInstalledPacks() {
    if (scanned_) return;
    scanned_ = true;
    const auto root = userPacksDir();
    if (!root.isDirectory()) return;
    for (const auto& dir : root.findChildFiles(juce::File::findDirectories, false)) {
        std::string err;
        if (!loadPackDir(dir.getFullPathName().toStdString(), err))
            juce::Logger::writeToLog("pack skipped: " + dir.getFileName()
                             + juce::String(" - ") + err);
    }
}

bool PackLoader::loadPackDir(const std::string& dir, std::string& error) {
    juce::File d{juce::String(dir)};
    PackManifest pm;
    if (!parsePackManifest(d.getChildFile("pack.json").loadFileAsString().toStdString(), pm)) {
        error = "missing or malformed pack.json";
        return false;
    }
    for (const auto& l : libs_)
        if (l.id == pm.id) return true;
    if (PackRegistry::instance().packById(pm.id)) {
        error = "a pack with id '" + pm.id + "' is already installed";
        return false;
    }

    const auto binDir = d.getChildFile("bin").getChildFile(juce::String(platformTag()));
    auto so = binDir.getChildFile("pack.so");
    if (!so.existsAsFile()) so = binDir.getChildFile("pack.dylib");
    if (!so.existsAsFile()) so = binDir.getChildFile("pack.dll");
    if (!so.existsAsFile()) {
        error = "no binary for this platform (" + platformTag() + ")";
        return false;
    }

    auto lib = std::make_unique<juce::DynamicLibrary>();
    if (!lib->open(so.getFullPathName())) {
        error = "could not load " + so.getFileName().toStdString();
        return false;
    }
    using AbiFn = int32_t (*)();
    using CreateFn = void* (*)(const char*);
    auto* abi = (AbiFn) lib->getFunction("hum_pack_abi");
    auto* create = (CreateFn) lib->getFunction("hum_pack_create");
    if (abi == nullptr || create == nullptr || abi() != HUM_PACK_ABI) {
        error = "pack ABI mismatch (host " + std::to_string(HUM_PACK_ABI) + ", pack "
                + (abi ? std::to_string(abi()) : std::string("?")) + ")";
        return false;
    }
    using ShapeFn = uint64_t (*)();
    if (auto* shape = (ShapeFn) lib->getFunction("hum_pack_shape");
        shape != nullptr && !shapeCompatible(shape(), error))
        return false;

    if (!PackRegistry::instance().loadPackDir(dir, false)) {
        error = "pack manifests failed to parse";
        return false;
    }
    if (const auto* pack = PackRegistry::instance().packById(pm.id))
        for (const auto& folder : pack->organisms)
            for (const auto& cls : folder.classes) {
                const std::string name = cls.className;
                if (const auto* owner = PackRegistry::instance().packOf(name);
                    owner != nullptr && owner->builtin && owner->manifest.id != pm.id) {
                    DBG("pack '" << pm.id << "': class '" << name << "' is owned by the "
                        "pre-installed '" << owner->manifest.id << "' pack - not overridden");
                    continue;
                }
                Registry::instance().registerClass(name, [create, name]() -> OrganismPtr {
                    return OrganismPtr(static_cast<Organism*>(create(name.c_str())));
                });
            }

    using LayoutFn = const char* (*)(const char*, const char*);
    if (auto* layout = (LayoutFn) lib->getFunction("hum_pack_layout_json"))
        Registry::instance().registerLayoutProvider(
            pm.id, [layout](const std::string& genId, const std::string& cls) -> std::string {
                const char* json = layout(genId.c_str(), cls.c_str());
                return json ? std::string(json) : std::string();
            });

    libs_.push_back({pm.id, std::move(lib)});
    return true;
}

bool PackLoader::humpackEntriesStayInside(const juce::ZipFile& zip, std::string& offending) {
    const auto root = juce::File::getSpecialLocation(juce::File::tempDirectory)
                          .getChildFile("humpack-entry-check");
    for (int i = 0; i < zip.getNumEntries(); ++i) {
        const auto name = zip.getEntry(i)->filename;
        const bool absolute = name.startsWithChar('/') || name.startsWithChar('\\')
                              || (name.length() > 1 && name[1] == ':');
        if (absolute || !root.getChildFile(name).isAChildOf(root)) {
            offending = name.toStdString();
            return false;
        }
    }
    return true;
}

bool PackLoader::installHumpack(const juce::File& bundle, std::string& error) {
    juce::ZipFile zip(bundle);
    if (zip.getNumEntries() == 0) { error = "not a valid .humpack (empty or unreadable)"; return false; }
    if (std::string bad; !humpackEntriesStayInside(zip, bad)) {
        error = "bundle entry '" + bad + "' would land outside the pack folder";
        return false;
    }

    PackManifest pm;
    {
        const int idx = zip.getIndexOfFileName("pack.json");
        if (idx < 0) { error = "bundle has no pack.json"; return false; }
        std::unique_ptr<juce::InputStream> in(zip.createStreamForEntry(idx));
        if (!in || !parsePackManifest(in->readEntireStreamAsString().toStdString(), pm)) {
            error = "bundle pack.json is malformed";
            return false;
        }
    }
    if (auto* existing = PackRegistry::instance().packById(pm.id)) {
        if (existing->builtin) {
            error = "'" + pm.id + "' is built in and cannot be replaced";
            return false;
        }
        unregisterPack(pm.id);
    }

    const auto dest = userPacksDir().getChildFile(juce::String(pm.id));
    const auto stage = userPacksDir().getChildFile("." + juce::String(pm.id) + ".staging");
    const auto retired = userPacksDir().getChildFile("." + juce::String(pm.id) + ".old");
    stage.deleteRecursively();
    retired.deleteRecursively();
    if (!stage.createDirectory()) { error = "cannot create the staging directory"; return false; }
    const auto result = zip.uncompressTo(stage, true);
    if (result.failed()) {
        stage.deleteRecursively();
        error = result.getErrorMessage().toStdString();
        return false;
    }
    if (dest.exists() && !dest.moveFileTo(retired)) {
        stage.deleteRecursively();
        error = "cannot move the old install aside";
        return false;
    }
    if (!stage.moveFileTo(dest)) {
        retired.moveFileTo(dest);
        error = "cannot move the new install into place";
        return false;
    }

    if (!loadPackDir(dest.getFullPathName().toStdString(), error)) {
        dest.deleteRecursively();
        retired.moveFileTo(dest);
        return false;
    }
    retired.deleteRecursively();
    return true;
}

bool PackLoader::uninstallPack(const std::string& id, std::string& error) {
    auto* pack = PackRegistry::instance().packById(id);
    if (!pack) { error = "no such pack: " + id; return false; }
    if (pack->builtin) { error = "built-in packs can be disabled but not uninstalled"; return false; }
    const juce::File dir{juce::String(pack->dir)};
    unregisterPack(id);
    if (dir.isDirectory() && dir.isAChildOf(userPacksDir())) dir.deleteRecursively();
    return true;
}

bool PackLoader::shapeCompatible(uint64_t packShape, std::string& error) {
    if (packShape == abiShape()) return true;
    error = "pack built against a different SDK layout (ABI " + std::to_string(HUM_PACK_ABI)
            + " on both sides, but the types crossing the boundary differ in size); "
              "rebuild the pack against this release's SDK";
    return false;
}

void PackLoader::unregisterPack(const std::string& id) {
    if (auto* pack = PackRegistry::instance().packById(id))
        for (const auto& folder : pack->organisms)
            for (const auto& cls : folder.classes) {
                const auto* owner = PackRegistry::instance().packOf(cls.className);
                if (owner != nullptr && owner->builtin && owner->manifest.id != id) continue;
                Registry::instance().unregisterClass(cls.className);
            }
    PackRegistry::instance().removePack(id);
    for (auto& l : libs_)
        if (l.id == id) l.id.clear();
}

}
