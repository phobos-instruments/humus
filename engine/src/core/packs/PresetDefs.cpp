// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/packs/PresetDefs.h"
#include "hum/FileBytes.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <system_error>

namespace hum {

namespace fs = std::filesystem;

namespace {

std::string trimmed(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace((unsigned char) s[a])) ++a;
    while (b > a && std::isspace((unsigned char) s[b - 1])) --b;
    return s.substr(a, b - a);
}

std::string lower(std::string s) {
    for (auto& c : s) c = (char) std::tolower((unsigned char) c);
    return s;
}

std::string stemOf(const std::string& path) {
    const auto name = utf8Text(utf8Path(path).filename());
    const auto dot = name.rfind('.');
    return dot == std::string::npos ? name : name.substr(0, dot);
}

std::vector<std::string> jsonFilesIn(const fs::path& dir) {
    std::vector<std::string> out;
    std::error_code ec;
    for (fs::directory_iterator it(dir, ec), end; !ec && it != end; it.increment(ec)) {
        const auto name = utf8Text(it->path().filename());
        if (name.size() >= 5 && lower(name.substr(name.size() - 5)) == ".json" && it->is_regular_file(ec))
            out.push_back(utf8Text(it->path()));
    }
    std::sort(out.begin(), out.end(), [](const std::string& a, const std::string& b) {
        const auto la = lower(a), lb = lower(b);
        return la != lb ? la < lb : a < b;
    });
    return out;
}

bool isFile(const std::string& path) {
    std::error_code ec;
    return fs::is_regular_file(path, ec);
}

}

std::vector<PresetDef> parsePresetDefs(const json::Value& v, const std::string& group) {
    std::vector<PresetDef> out;
    for (const auto& e : v.items()) {
        PresetDef pd;
        pd.name = trimmed(e["name"].text());
        pd.group = group;
        if (pd.name.empty()) continue;
        for (const auto& [name, value] : e["values"].members()) {
            if (value.isString())
                pd.texts.emplace_back(name, value.text());
            else
                pd.values.emplace_back(name, value.number());
        }
        for (const auto& [name, value] : e["texts"].members()) pd.texts.emplace_back(name, value.text());
        out.push_back(std::move(pd));
    }
    return out;
}

std::string presetGroupOf(const std::string& path) {
    const auto n = trimmed(stemOf(path));
    size_t i = 0;
    while (i < n.size() && n[i] >= '0' && n[i] <= '9') ++i;
    return (i > 0 && i < n.size() && n[i] == ' ') ? trimmed(n.substr(i + 1)) : n;
}

std::string presetGroupFile(const std::string& dir, const std::string& className, const std::string& group) {
    if (group.empty()) return utf8Text(utf8Path(dir) / utf8Path(className + ".json"));
    const auto cdir = utf8Path(dir) / utf8Path(className);
    for (const auto& f : jsonFilesIn(cdir))
        if (presetGroupOf(f) == group) return f;
    return utf8Text(cdir / utf8Path(group + ".json"));
}

std::vector<PresetDef> loadPresetTree(const std::string& dir, const std::string& className) {
    std::vector<PresetDef> out;
    if (const auto flat = presetGroupFile(dir, className, {}); isFile(flat))
        for (auto& pd : parsePresetDefs(json::parseFile(flat))) out.push_back(std::move(pd));
    for (const auto& f : jsonFilesIn(utf8Path(dir) / utf8Path(className)))
        for (auto& pd : parsePresetDefs(json::parseFile(f), presetGroupOf(f))) out.push_back(std::move(pd));
    return out;
}

}
