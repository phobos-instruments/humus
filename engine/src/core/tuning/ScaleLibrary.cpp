// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/tuning/ScaleLibrary.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <set>

#include "core/app/AppPaths.h"
#include "hum/Number.h"

namespace hum {

std::string latin1FallbackToUtf8(const std::string& raw) {
    {
        const auto* p = (const unsigned char*) raw.data();
        const auto* end = p + raw.size();
        bool ok = true;
        while (p < end && ok) {
            if (*p < 0x80) { ++p; continue; }
            int n = (*p >> 5) == 0x6 ? 1 : (*p >> 4) == 0xE ? 2 : (*p >> 3) == 0x1E ? 3 : -1;
            if (n < 0 || p + n >= end) { ok = false; break; }
            for (int i = 1; i <= n; ++i)
                if ((p[i] & 0xC0) != 0x80) { ok = false; break; }
            p += n + 1;
        }
        if (ok) return raw;
    }
    std::string out;
    out.reserve(raw.size() * 2);
    for (const unsigned char c : raw) {
        if (c < 0x80) {
            out += (char) c;
        } else {
            out += (char) (0xC0 | (c >> 6));
            out += (char) (0x80 | (c & 0x3F));
        }
    }
    return out;
}

juce::File userScalesDir() { return userContentRoot().getChildFile("Scales"); }

std::vector<juce::File> scaleLibraryRoots() {
    std::vector<juce::File> roots;
    if (const auto legacy = appDataDir().getChildFile("scales"); legacy.isDirectory())
        roots.push_back(legacy);
    for (const auto& d : assetSearchPath("Scales")) roots.push_back(d);
    return roots;
}

namespace {

double intervalCents(const std::string& line) {
    const char* s = line.c_str();
    const char* end = nullptr;
    const double p = scanDouble(s, &end);
    if (end == s) return 0.0;
    for (const char* c = s; c != end; ++c)
        if (*c == '.') return p;
    if (*end == '/') {
        const char* qs = end + 1;
        const double q = scanDouble(qs, &end);
        if (end == qs || p <= 0.0 || q <= 0.0) return 0.0;
        return 1200.0 * std::log2(p / q);
    }
    return p > 0.0 ? 1200.0 * std::log2(p) : 0.0;
}

void scanRoot(const juce::File& root, std::set<std::string>& seen,
              std::vector<ScaleInfo>& out) {
    if (!root.isDirectory()) return;
    for (const auto& entry :
         juce::RangedDirectoryIterator(root, true, "*.scl")) {
        const juce::File f = entry.getFile();

        ScaleInfo info;
        info.path = f.getFullPathName().toStdString();
        info.name = f.getFileNameWithoutExtension().toStdString();
        const auto rel = f.getParentDirectory().getRelativePathFrom(root);
        info.collection = (rel == "." ? juce::String() : rel)
                              .replaceCharacter('\\', '/')
                              .toStdString();

        const std::string key = info.collection + "/" + info.name;
        if (!seen.insert(key).second) continue;

        juce::MemoryBlock mb;
        f.loadFileAsData(mb);
        const std::string text = latin1FallbackToUtf8(
            std::string((const char*) mb.getData(), mb.getSize()));

        int realLine = 0, wantIntervals = -1, gotIntervals = 0;
        std::string lastInterval;
        std::size_t pos = 0;
        while (pos <= text.size()) {
            std::size_t eol = text.find('\n', pos);
            if (eol == std::string::npos) eol = text.size();
            std::size_t a = pos, b = eol;
            pos = eol + 1;
            while (a < b && (text[a] == ' ' || text[a] == '\t' || text[a] == '\r')) ++a;
            while (b > a && (text[b - 1] == ' ' || text[b - 1] == '\t' || text[b - 1] == '\r')) --b;
            if (a < b && text[a] == '!') continue;
            ++realLine;
            const std::string line = text.substr(a, b - a);
            if (realLine == 1) {
                info.description = line;
            } else if (realLine == 2) {
                char* end = nullptr;
                const long n = std::strtol(line.c_str(), &end, 10);
                if (end != line.c_str() && n > 0 && n < 4096) {
                    info.degrees = (int) n;
                    wantIntervals = (int) n;
                } else {
                    break;
                }
            } else if (wantIntervals > 0 && gotIntervals < wantIntervals) {
                if (!line.empty()) lastInterval = line;
                ++gotIntervals;
            } else {
                break;
            }
        }
        if (!lastInterval.empty()) info.periodCents = intervalCents(lastInterval);
        out.push_back(std::move(info));
    }
}

}

std::vector<ScaleInfo> scanScaleDirs(const std::vector<juce::File>& roots) {
    std::vector<ScaleInfo> out;
    std::set<std::string> seen;
    for (const auto& root : roots) scanRoot(root, seen, out);
    std::sort(out.begin(), out.end(), [](const ScaleInfo& a, const ScaleInfo& b) {
        return a.collection != b.collection ? a.collection < b.collection : a.name < b.name;
    });
    return out;
}

}
