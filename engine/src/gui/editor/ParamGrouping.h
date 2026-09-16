// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <cctype>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace hum {

inline std::string paramRootOf(const std::string& name) {
    static const std::string seps = " -_:./";
    std::vector<std::string> tok;
    std::string cur;
    for (char ch : name) {
        if (seps.find(ch) != std::string::npos) {
            if (!cur.empty()) tok.push_back(cur);
            cur.clear();
        } else {
            cur += ch;
        }
    }
    if (!cur.empty()) tok.push_back(cur);
    if (tok.size() < 2) return {};
    std::string root = tok[0];
    if (tok[1].find_first_not_of("0123456789") == std::string::npos) root += " " + tok[1];
    return root;
}

struct ParamSection {
    std::string title;
    std::vector<int> rows;
};

inline std::vector<ParamSection> planParamSections(
        const std::vector<std::pair<std::string, std::string>>& params) {
    struct Info { std::string label, key; bool tree = false; };
    std::vector<Info> info(params.size());
    std::map<std::string, int> count;
    for (size_t i = 0; i < params.size(); ++i) {
        Info& f = info[i];
        f.tree = !params[i].second.empty();
        f.label = f.tree ? params[i].second : paramRootOf(params[i].first);
        for (char ch : f.label) f.key += (char) std::tolower((unsigned char) ch);
        if (!f.label.empty()) ++count[f.key];
    }
    std::vector<ParamSection> out;
    std::map<std::string, size_t> secOf;
    for (size_t i = 0; i < params.size(); ++i) {
        const Info& f = info[i];
        if (!f.label.empty() && count[f.key] >= (f.tree ? 2 : 3)) {
            auto it = secOf.find(f.key);
            if (it == secOf.end()) {
                it = secOf.emplace(f.key, out.size()).first;
                out.push_back({f.label, {}});
            }
            out[it->second].rows.push_back((int) i);
        } else {
            if (out.empty() || !out.back().title.empty()) out.push_back({});
            out.back().rows.push_back((int) i);
        }
    }
    return out;
}

inline std::vector<std::string> disambiguateNames(
        const std::vector<std::pair<std::string, int>>& raw) {
    std::vector<std::string> out;
    std::map<std::string, int> seen;
    for (const auto& [name0, idx] : raw) {
        std::string name = name0.empty() ? "Param" : name0;
        if (++seen[name] > 1 || name == "Param") name += " #" + std::to_string(idx);
        out.push_back(name);
    }
    return out;
}

inline std::vector<char> collapseAliasedRows(
        const std::vector<std::array<std::string, 3>>& rows) {
    std::vector<char> keep(rows.size(), 1);
    std::map<std::string, std::vector<size_t>> byId;
    for (size_t i = 0; i < rows.size(); ++i)
        if (!rows[i][0].empty()) byId[rows[i][0]].push_back(i);
    for (const auto& [id, members] : byId) {
        if (members.size() < 2) continue;
        size_t keeper = members[0];
        for (size_t m : members)
            if (rows[m][1] == rows[m][2]) { keeper = m; break; }
        for (size_t m : members) keep[m] = m == keeper;
    }
    return keep;
}

inline std::string paramSearchKey(const std::string& s) {
    std::string k;
    k.reserve(s.size());
    for (char ch : s) k += (char) std::tolower((unsigned char) ch);
    return k;
}

struct SectionView {
    bool showHeader = false;
    std::vector<int> rows;
};

inline std::vector<SectionView> planVisibleRows(const std::vector<ParamSection>& secs,
                                                const std::vector<std::string>& rowNames,
                                                const std::vector<char>& folded,
                                                const std::string& filterRaw) {
    const std::string needle = paramSearchKey(filterRaw);
    std::vector<SectionView> out(secs.size());
    for (size_t s = 0; s < secs.size(); ++s) {
        const ParamSection& sec = secs[s];
        SectionView& v = out[s];
        if (needle.empty()) {
            v.showHeader = !sec.title.empty();
            if (!(s < folded.size() && folded[s])) v.rows = sec.rows;
            continue;
        }
        const bool whole = !sec.title.empty()
                        && paramSearchKey(sec.title).find(needle) != std::string::npos;
        for (int i : sec.rows)
            if (whole || (i >= 0 && i < (int) rowNames.size()
                          && paramSearchKey(rowNames[(size_t) i]).find(needle)
                                 != std::string::npos))
                v.rows.push_back(i);
        v.showHeader = !sec.title.empty() && !v.rows.empty();
    }
    return out;
}

}
