// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/app/LanguageCatalogue.h"

#include <algorithm>
#include <cstdint>

#include "core/packs/PackRegistry.h"
#include "hum/FileBytes.h"

namespace hum {

namespace {

constexpr const char* kPrefix = "app.";
constexpr const char* kSuffix = ".po";

bool endsWith(const std::string& s, const std::string& tail) {
    return s.size() >= tail.size() && s.compare(s.size() - tail.size(), tail.size(), tail) == 0;
}

}

std::string interfaceCodeOf(const std::string& embeddedName) {
    const std::string prefix(kPrefix), suffix(kSuffix);
    if (embeddedName.rfind(prefix, 0) != 0 || !endsWith(embeddedName, suffix)) return {};
    if (embeddedName.size() <= prefix.size() + suffix.size()) return {};
    return embeddedName.substr(prefix.size(), embeddedName.size() - prefix.size() - suffix.size());
}

std::vector<LanguageChoice> languageChoices(const std::vector<std::pair<std::string, std::string>>& interfaces) {
    std::vector<LanguageChoice> out;
    for (const auto& [code, text] : interfaces) {
        const auto name = Catalogue::fromPo(text).languageName();
        out.push_back({code, name.empty() ? code : name});
    }
    std::sort(out.begin(), out.end(), [](const LanguageChoice& a, const LanguageChoice& b) { return a.name < b.name; });
    return out;
}

std::string packStringsPath(const std::string& packDir, const std::string& code) {
    return utf8Text(utf8Path(packDir) / "translations" / utf8Path(code) / "organisms.po");
}

void addPackStrings(Catalogue& into, const std::vector<std::string>& packDirs, const std::string& code) {
    for (const auto& dir : packDirs) {
        std::vector<std::uint8_t> bytes;
        if (readFileBytes(packStringsPath(dir, code), bytes))
            into.add(Catalogue::fromPo(std::string(bytes.begin(), bytes.end())));
    }
}

std::vector<std::string> loadedPackDirs() {
    auto& registry = PackRegistry::instance();
    registry.loadBuiltinPacks();
    std::vector<std::string> out;
    for (const auto& pack : registry.packs()) out.push_back(pack.dir);
    return out;
}

}
