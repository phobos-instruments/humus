// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include <juce_core/juce_core.h>

#include "core/app/AppPaths.h"
#include "core/packs/PackRegistry.h"

namespace hum::catalogue {

struct Kind {
    std::string id;
    std::string wildcard;
    std::string holds;
    std::string badge;
    std::vector<std::pair<std::string, std::string>> badges;
};

inline const std::vector<Kind>& kinds() {
    static const std::vector<Kind> all = {
        {"Impulses", "*.wav;*.aif;*.aiff;*.flac",
         "Impulse responses: a recording of a room, a plate or a spring.", "IR", {}},
        {"Samples", "*.wav;*.aif;*.aiff;*.flac",
         "One sound per file.", "ONE-SHOT", {}},
        {"Banks", "*.sf2;*.syx;*.opm;*.wopl;*.wopn;*.tfi;*.dmp",
         "Many instruments in one file.", "BANK",
         {{".sf2", "SF2"}, {".syx", "6-OP"}, {".opm", "4-OP"}, {".wopl", "2-OP"},
          {".wopn", "CHIP"}, {".tfi", "CHIP"}, {".dmp", "CHIP"}}},
        {"Scores", "*.mid;*.midi;*.kar;*.mus",
         "Scores: a song written out for instruments, not recorded.", "SCORE",
         {{".mid", "MIDI"}, {".midi", "MIDI"}, {".kar", "KARAOKE"}, {".mus", "MUS"}}},
        {"Scales", "*.scl",
         "Scala tuning files.", "SCL", {}},
        {"Shaders", "*.frag;*.fs;*.glsl;*.fsh;*.synScene",
         "Fragment shader scenes: a .frag or .fs file, or a .synScene folder.",
         "SCENE", {}},
    };
    return all;
}

inline bool matches(const juce::File& f, const std::string& wildcards) {
    const auto name = f.getFileName();
    for (const auto& one : juce::StringArray::fromTokens(
             juce::String(juce::CharPointer_UTF8(wildcards.c_str())), ";", ""))
        if (name.matchesWildcard(one.trim(), true)) return true;
    return false;
}

inline std::string badgeFor(const juce::File& f, const Kind& kind) {
    const auto ext = f.getFileExtension().toLowerCase();
    for (const auto& [suffix, badge] : kind.badges)
        if (ext == juce::String(suffix)) return badge;
    return kind.badge;
}

inline const Kind* find(const std::string& id) {
    const juce::String want(juce::CharPointer_UTF8(id.c_str()));
    for (const auto& k : kinds())
        if (want.equalsIgnoreCase(juce::String(k.id))) return &k;
    return nullptr;
}

inline juce::File plantUserFolders() {
    const auto dir = userContentRoot();
    for (const auto& kind : kinds()) {
        const auto sub = dir.getChildFile(kind.id);
        sub.createDirectory();
        const auto note = sub.getChildFile("README.txt");
        const juce::String text =
            "This folder is yours.\r\n\r\n"
            + juce::String(juce::CharPointer_UTF8(kind.holds.c_str())) + "\r\n\r\n"
            "It starts empty on purpose. The ones Humus ships with live inside "
            "the application and are always available - they are not copied "
            "here, so an update can improve them without touching your own.\r\n\r\n"
            "Anything you drop in here appears alongside them, and wins if it "
            "has the same name.\r\n";
        const auto had = note.existsAsFile() ? note.loadFileAsString() : juce::String();
        if (had == text) continue;
        if (had.isEmpty() || had.startsWith("This folder is yours.")) note.replaceWithText(text);
    }
    return dir;
}

inline std::string relativeTo(const juce::File& f, const juce::File& root) {
    return f.getRelativePathFrom(root).replaceCharacter('\\', '/').toStdString();
}

inline std::vector<juce::File> roots(const Kind& kind, const std::string& organism = {}) {
    std::vector<juce::File> out;
    for (const auto& d : assetSearchPath(kind.id)) out.push_back(d);
    if (!organism.empty())
        if (const auto* folder = PackRegistry::instance().folderOf(organism))
            out.push_back(juce::File(juce::String(folder->dir)).getChildFile("banks"));
    return out;
}

struct Entry {
    juce::File file;
    std::string ref;
    std::string name;
    std::string group;
};

inline std::string refFor(const juce::File& f, const Kind& kind,
                          const std::string& organism = {}) {
    for (const auto& root : roots(kind, organism))
        if (f.isAChildOf(root))
            return std::string(kAssetScheme) + kind.id + "/" + relativeTo(f, root);
    return f.getFullPathName().toStdString();
}

inline std::vector<Entry> scan(const Kind& kind, const std::string& organism = {}) {
    std::vector<Entry> out;
    juce::StringArray seen;
    for (const auto& root : roots(kind, organism)) {
        if (!root.isDirectory()) continue;
        const juce::String wc(juce::CharPointer_UTF8(kind.wildcard.c_str()));
        auto found = root.findChildFiles(juce::File::findFiles, true, wc);
        found.addArray(root.findChildFiles(juce::File::findDirectories, true, wc));
        for (const auto& f : found) {
            const auto rel = relativeTo(f, root);
            if (rel.find(".synScene/") != std::string::npos) continue;
            if (seen.contains(juce::String(juce::CharPointer_UTF8(rel.c_str())))) continue;
            seen.add(juce::String(juce::CharPointer_UTF8(rel.c_str())));
            const auto folder = relativeTo(f.getParentDirectory(), root);
            out.push_back({f, std::string(kAssetScheme) + kind.id + "/" + rel,
                           f.getFileNameWithoutExtension().toStdString(),
                           folder == "." ? std::string() : folder});
        }
    }
    std::sort(out.begin(), out.end(), [](const Entry& a, const Entry& b) {
        return a.group != b.group ? a.group < b.group : a.name < b.name;
    });
    return out;
}

inline std::string resolve(const std::string& ref, const std::string& organism = {}) {
    const std::string scheme = kAssetScheme;
    if (ref.rfind(scheme, 0) != 0) return ref;
    const auto rest = ref.substr(scheme.size());
    const auto slash = rest.find('/');
    if (slash == std::string::npos) return ref;
    const auto* kind = find(rest.substr(0, slash));
    if (kind == nullptr) return ref;
    const auto want = rest.substr(slash + 1);
    for (const auto& root : roots(*kind, organism))
        if (const auto f = root.getChildFile(juce::String(juce::CharPointer_UTF8(want.c_str())));
            f.exists())
            return f.getFullPathName().toStdString();
    return ref;
}

}
