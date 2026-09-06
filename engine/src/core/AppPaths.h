#pragma once
#include <vector>

#include <juce_core/juce_core.h>

namespace hum {

inline juce::File appDataDir() {
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Humus");
}

inline juce::File& userRootForTesting() {
    static juce::File f;
    return f;
}

inline juce::File userLibraryRoot() {
    if (const auto& t = userRootForTesting(); t != juce::File()) return t;
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("Humus");
}

inline juce::File userPatchesDir() { return userLibraryRoot().getChildFile("Patches"); }

inline void mergeUserFolderInto(const juce::File& from, const juce::File& to) {
    if (!from.exists()) return;
    if (!to.exists()) {
        to.getParentDirectory().createDirectory();
        from.moveFileTo(to);
        return;
    }
    if (!from.isDirectory() || !to.isDirectory()) return;
    for (const auto& c : from.findChildFiles(juce::File::findFilesAndDirectories, false))
        mergeUserFolderInto(c, to.getChildFile(c.getFileName()));
    from.deleteFile();
}

inline void migrateLegacyUserFolders(const juce::File& musicDir,
                                     const juce::File& docsDir,
                                     const juce::File& appData) {
    const auto root = docsDir.getChildFile("Humus");
    mergeUserFolderInto(musicDir.getChildFile("Humus"), root);
    mergeUserFolderInto(musicDir.getChildFile("Humus Library"), root.getChildFile("Library"));
    mergeUserFolderInto(musicDir.getChildFile("Humus Recordings"), root.getChildFile("Recordings"));
    mergeUserFolderInto(musicDir.getChildFile("Humus Samples"),
                        root.getChildFile("Library").getChildFile("Samples"));
    mergeUserFolderInto(appData.getChildFile("presets"), root.getChildFile("Presets"));
    mergeUserFolderInto(appData.getChildFile("banks"),
                        root.getChildFile("Library").getChildFile("Banks"));

    const auto legacy = root.getChildFile("assets");
    const auto lib = root.getChildFile("Library");
    mergeUserFolderInto(legacy.getChildFile("scales"), lib.getChildFile("Scales"));
    mergeUserFolderInto(legacy.getChildFile("shaders"), lib.getChildFile("Shaders"));
    mergeUserFolderInto(legacy.getChildFile("banks").getChildFile("Verbatim"),
                        lib.getChildFile("Impulses"));
    mergeUserFolderInto(legacy.getChildFile("banks").getChildFile("Sampler"),
                        lib.getChildFile("Samples"));
    mergeUserFolderInto(legacy.getChildFile("banks").getChildFile("pH"),
                        lib.getChildFile("Banks"));
}

inline std::string& configuredContentRoot() {
    static std::string r;
    return r;
}

inline juce::File userContentRoot() {
    if (!configuredContentRoot().empty())
        return juce::File(juce::String(juce::CharPointer_UTF8(configuredContentRoot().c_str())));
    return userLibraryRoot().getChildFile("Library");
}

inline juce::File bundledAssetsDir() {
    const auto exe = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    const auto dir = exe.getParentDirectory();
    const juce::File candidates[] = {
        dir.getChildFile("assets"),
        dir.getParentDirectory().getChildFile("Resources").getChildFile("assets"),
    };
    for (const auto& c : candidates)
        if (c.isDirectory()) return c;
    return {};
}

inline juce::File assetKindDir(const juce::File& parent, const juce::String& kind) {
    if (const auto exact = parent.getChildFile(kind); exact.isDirectory()) return exact;
    for (const auto& d : parent.findChildFiles(juce::File::findDirectories, false))
        if (d.getFileName().equalsIgnoreCase(kind)) return d;
    return {};
}

inline std::vector<juce::File> packAssetRoots(const juce::String& kind) {
    std::vector<juce::File> out;
    const auto addPacksUnder = [&](const juce::File& root) {
        if (!root.isDirectory()) return;
        for (const auto& pack : root.findChildFiles(juce::File::findDirectories, false))
            if (pack.getChildFile("pack.json").existsAsFile())
                if (const auto d = assetKindDir(pack.getChildFile("assets"), kind);
                    d != juce::File())
                    out.push_back(d);
    };
    const auto exeDir =
        juce::File::getSpecialLocation(juce::File::currentExecutableFile)
            .getParentDirectory();
    addPacksUnder(
        exeDir.getParentDirectory().getChildFile("Resources").getChildFile("packs"));
    addPacksUnder(exeDir.getChildFile("packs"));
    addPacksUnder(exeDir.getParentDirectory().getChildFile("packs"));
#ifdef HUM_PACKS_DIR
    addPacksUnder(juce::File(juce::String(HUM_PACKS_DIR)));
#endif
    addPacksUnder(appDataDir().getChildFile("packs"));
    return out;
}

inline std::vector<juce::File> assetSearchPath(const juce::String& kind) {
    std::vector<juce::File> out;
    if (const auto mine = assetKindDir(userContentRoot(), kind); mine != juce::File())
        out.push_back(mine);
    if (const auto shipped = bundledAssetsDir(); shipped != juce::File())
        if (const auto d = assetKindDir(shipped, kind); d != juce::File()) out.push_back(d);
#ifdef HUM_PACKS_DIR
    if (const auto repo = assetKindDir(juce::File(HUM_PACKS_DIR).getParentDirectory()
                                           .getChildFile("assets"), kind);
        repo != juce::File())
        out.push_back(repo);
#endif
    for (const auto& d : packAssetRoots(kind)) out.push_back(d);
    return out;
}

inline constexpr const char* kAssetScheme = "asset:";

inline juce::File resolveAssetRef(const juce::String& ref) {
    if (!ref.startsWithIgnoreCase(kAssetScheme)) return {};
    const auto rel = ref.substring((int) juce::String(kAssetScheme).length())
                        .trimCharactersAtStart("/");
    const auto kind = rel.upToFirstOccurrenceOf("/", false, false);
    const auto name = rel.fromFirstOccurrenceOf("/", false, false);
    if (kind.isEmpty() || name.isEmpty()) return {};
    for (const auto& dir : assetSearchPath(kind))
        if (const auto f = dir.getChildFile(name); f.exists()) return f;
    return {};
}

}
