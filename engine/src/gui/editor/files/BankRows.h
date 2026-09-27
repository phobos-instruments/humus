// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

namespace hum::files {

enum class BankSource { Factory, Yours, Imported, Bundled, Pack };

struct BankRow {
    std::string ref;
    std::string name;
    std::string kind;
    BankSource source = BankSource::Bundled;
    bool recent = false;
    bool missing = false;
    std::string detail;
};

inline bool bankRowForgettable(const BankRow& row) {
    return row.missing || row.source == BankSource::Imported;
}

enum class BankBucket { All, Recent, Yours, Imported, Bundled, Pack, Factory, Missing };

inline const std::vector<BankBucket>& bankBucketOrder() {
    static const std::vector<BankBucket> all = {
        BankBucket::All,     BankBucket::Recent,  BankBucket::Yours,   BankBucket::Imported,
        BankBucket::Bundled, BankBucket::Pack,    BankBucket::Factory, BankBucket::Missing};
    return all;
}

inline bool bankRowIn(const BankRow& row, BankBucket bucket) {
    switch (bucket) {
        case BankBucket::All:      return true;
        case BankBucket::Recent:   return row.recent && !row.missing;
        case BankBucket::Missing:  return row.missing;
        case BankBucket::Yours:    return !row.missing && row.source == BankSource::Yours;
        case BankBucket::Imported: return !row.missing && row.source == BankSource::Imported;
        case BankBucket::Bundled:  return !row.missing && row.source == BankSource::Bundled;
        case BankBucket::Pack:     return !row.missing && row.source == BankSource::Pack;
        case BankBucket::Factory:  return !row.missing && row.source == BankSource::Factory;
    }
    return false;
}

inline std::string bankBucketName(BankBucket bucket) {
    switch (bucket) {
        case BankBucket::All:      return "All";
        case BankBucket::Recent:   return "Recent";
        case BankBucket::Yours:    return "Yours";
        case BankBucket::Imported: return "Imported";
        case BankBucket::Bundled:  return "Bundled";
        case BankBucket::Pack:     return "Pack";
        case BankBucket::Factory:  return "Factory";
        case BankBucket::Missing:  return "Missing";
    }
    return {};
}

struct BankBucketCount {
    BankBucket bucket = BankBucket::All;
    int count = 0;
};

inline std::vector<BankBucketCount> bankBuckets(const std::vector<BankRow>& rows) {
    std::vector<BankBucketCount> out;
    for (const auto bucket : bankBucketOrder()) {
        int count = 0;
        for (const auto& row : rows)
            if (bankRowIn(row, bucket)) ++count;
        if (count > 0 || bucket == BankBucket::All) out.push_back({bucket, count});
    }
    return out;
}

inline std::string lowerAscii(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (const char c : text) out += (char) (c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c);
    return out;
}

inline std::vector<std::string> bankKinds(const std::vector<BankRow>& rows) {
    std::vector<std::string> out;
    for (const auto& row : rows) {
        if (row.kind.empty()) continue;
        bool had = false;
        for (const auto& k : out) had = had || k == row.kind;
        if (!had) out.push_back(row.kind);
    }
    std::sort(out.begin(), out.end());
    return out;
}

inline bool bankRowMatches(const BankRow& row, const std::string& query) {
    if (query.empty()) return true;
    return lowerAscii(row.name).find(query) != std::string::npos
           || lowerAscii(row.detail).find(query) != std::string::npos;
}

inline constexpr std::size_t kBadgeFits = 6;
inline constexpr std::size_t kBadgeStem = 5;

inline std::string bankBadge(const std::string& kind) {
    if (kind.size() <= kBadgeFits) return kind;
    std::string initials;
    bool wordStart = true;
    for (const char c : kind) {
        const bool gap = c == ' ' || c == '-' || c == '_';
        if (wordStart && !gap) initials += c;
        wordStart = gap;
    }
    return initials.size() > 1 ? initials : kind.substr(0, kBadgeStem) + ".";
}

inline std::string scaleDetail(int degrees, const std::string& description) {
    std::string out = degrees > 0 ? std::to_string(degrees) + (degrees == 1 ? " note" : " notes") : std::string();
    if (!description.empty()) out += (out.empty() ? "" : " - ") + description;
    return out;
}

inline std::vector<BankRow> filterBankRows(const std::vector<BankRow>& rows, BankBucket bucket,
                                           const std::string& query,
                                           const std::string& kind = {}) {
    const auto want = lowerAscii(query);
    std::vector<BankRow> out;
    for (const auto& row : rows)
        if (bankRowIn(row, bucket) && bankRowMatches(row, want)
            && (kind.empty() || row.kind == kind))
            out.push_back(row);
    return out;
}

}
