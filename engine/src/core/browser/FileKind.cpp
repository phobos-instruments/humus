// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/browser/FileKind.h"

#include <algorithm>
#include <array>
#include <cctype>

#include "core/project/ProjectFolder.h"
#include "hum/FileBytes.h"

namespace hum::browser {

namespace {

constexpr std::array<KindText, 12> kTexts{{
    {Kind::Other, "other", "FILE"},
    {Kind::Project, "project", "PROJECT"},
    {Kind::Patch, "patch", "PATCH"},
    {Kind::Sound, "sample", "SOUND"},
    {Kind::Impulse, "ir", "IR"},
    {Kind::Bank, "bank", "BANK"},
    {Kind::Midi, "midi", "MIDI"},
    {Kind::Scale, "scale", "SCL"},
    {Kind::Shader, "shader", "SCENE"},
    {Kind::Video, "video", "VIDEO"},
    {Kind::Preset, "preset", "PRESET"},
    {Kind::Folder, "folder", "FOLDER"},
}};

struct Extension {
    const char* ext;
    Kind kind;
};

constexpr std::array<Extension, 33> kExtensions{{
    {".hum", Kind::Patch}, {".amh", Kind::Patch},
    {".wav", Kind::Sound}, {".aif", Kind::Sound}, {".aiff", Kind::Sound}, {".flac", Kind::Sound},
    {".mp3", Kind::Sound}, {".ogg", Kind::Sound}, {".m4a", Kind::Sound},
    {".sf2", Kind::Bank}, {".nsmp3", Kind::Bank}, {".nsmp4", Kind::Bank}, {".syx", Kind::Bank},
    {".opm", Kind::Bank}, {".wopl", Kind::Bank}, {".wopn", Kind::Bank}, {".tfi", Kind::Bank}, {".dmp", Kind::Bank},
    {".mid", Kind::Midi}, {".midi", Kind::Midi}, {".kar", Kind::Midi}, {".mus", Kind::Midi}, {".seq", Kind::Midi},
    {".scl", Kind::Scale},
    {".frag", Kind::Shader}, {".fs", Kind::Shader}, {".glsl", Kind::Shader}, {".fsh", Kind::Shader},
    {".mp4", Kind::Video}, {".mov", Kind::Video}, {".m4v", Kind::Video},
    {".humpreset", Kind::Preset},
    {".synscene", Kind::Shader},
}};

bool sameFolderName(const std::string& path, std::string_view folder) {
    auto p = utf8Path(path).parent_path();
    for (; !p.empty() && p != p.parent_path(); p = p.parent_path())
        if (utf8Text(p.filename()) == folder) return true;
    return false;
}

}

const KindText* kindTexts(int& count) {
    count = (int) kTexts.size();
    return kTexts.data();
}

const char* kindWord(Kind kind) {
    for (const auto& t : kTexts)
        if (t.kind == kind) return t.word;
    return kTexts[0].word;
}

const char* kindBadge(Kind kind) {
    for (const auto& t : kTexts)
        if (t.kind == kind) return t.badge;
    return kTexts[0].badge;
}

bool parseKindWord(std::string_view word, Kind& out) {
    std::string lower(word);
    for (auto& c : lower) c = (char) std::tolower((unsigned char) c);
    for (const auto& t : kTexts)
        if (lower == t.word) { out = t.kind; return true; }
    return false;
}

Kind kindOfFile(const std::string& path) {
    const auto ext = lowerExtension(path);
    for (const auto& e : kExtensions) {
        if (ext != e.ext) continue;
        if (e.kind == Kind::Sound && sameFolderName(path, "Impulses")) return Kind::Impulse;
        return e.kind;
    }
    return Kind::Other;
}

Kind kindOfFolder(const std::string& dir) {
    if (isProjectFolder(dir)) return Kind::Project;
    return lowerExtension(dir) == ".synscene" ? Kind::Shader : Kind::Other;
}

bool isProjectFolder(const std::string& dir) { return project::isProject(dir); }

namespace {

std::vector<std::string> extensionsIn(const std::string& patterns, bool& any) {
    std::vector<std::string> out;
    any = false;
    size_t from = 0;
    while (from <= patterns.size()) {
        const auto to = std::min(patterns.find(';', from), patterns.size());
        auto one = patterns.substr(from, to - from);
        while (!one.empty() && one.front() == ' ') one.erase(one.begin());
        while (!one.empty() && one.back() == ' ') one.pop_back();
        if (one.rfind("*.", 0) == 0 && one.find_first_of("*?", 2) == std::string::npos) {
            for (auto& c : one) c = (char) std::tolower((unsigned char) c);
            out.push_back(one.substr(1));
        } else if (!one.empty()) {
            any = true;
        }
        from = to + 1;
    }
    return out;
}

}

bool matchesPatterns(const std::string& path, const std::string& patterns) {
    bool any = false;
    const auto exts = extensionsIn(patterns, any);
    if (any || exts.empty()) return true;
    const auto ext = lowerExtension(path);
    return std::find(exts.begin(), exts.end(), ext) != exts.end();
}

std::vector<Kind> kindsInPatterns(const std::string& patterns) {
    bool any = false;
    std::vector<Kind> out;
    for (const auto& ext : extensionsIn(patterns, any)) {
        const auto k = kindOfFile("x" + ext);
        if (k != Kind::Other && std::find(out.begin(), out.end(), k) == out.end()) out.push_back(k);
    }
    return out;
}

std::string fileName(const std::string& path) { return utf8Text(utf8Path(path).filename()); }

std::string parentOf(const std::string& path) { return utf8Text(utf8Path(path).parent_path()); }

}
