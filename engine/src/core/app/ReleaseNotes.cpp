// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/app/ReleaseNotes.h"

#include <algorithm>
#include <cstddef>
#include <sstream>

#include "core/app/Versions.h"

namespace hum {

namespace {

bool isReleaseHeading(const std::string& line, std::string& version, std::string& date) {
    if (line.rfind("## [", 0) != 0) return false;
    const std::size_t close = line.find(']', 4);
    if (close == std::string::npos) return false;
    version = line.substr(4, close - 4);
    if (version.empty() || version[0] < '0' || version[0] > '9') return false;
    const std::size_t dash = line.find('-', close);
    date = dash == std::string::npos ? std::string() : line.substr(dash + 1);
    date.erase(0, date.find_first_not_of(' '));
    return true;
}

std::string trimmedLines(const std::vector<std::string>& lines) {
    std::size_t first = 0, last = lines.size();
    while (first < last && lines[first].empty()) ++first;
    while (last > first && lines[last - 1].empty()) --last;
    std::string out;
    for (std::size_t i = first; i < last; ++i) out += lines[i] + "\n";
    return out;
}

}

std::vector<ReleaseNotes> readChangelog(const std::string& markdown) {
    std::vector<ReleaseNotes> out;
    std::vector<std::string> body;
    bool inRelease = false;
    auto finish = [&] {
        if (inRelease) out.back().body = trimmedLines(body);
        body.clear();
    };
    std::istringstream in(markdown);
    for (std::string line; std::getline(in, line);) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.rfind("## ", 0) == 0) {
            finish();
            ReleaseNotes notes;
            inRelease = isReleaseHeading(line, notes.version, notes.date);
            if (inRelease) out.push_back(std::move(notes));
            continue;
        }
        if (inRelease) body.push_back(line);
    }
    finish();
    return out;
}

std::vector<ReleaseNotes> notesBetween(const std::vector<ReleaseNotes>& changelog,
                                       const std::string& lastSeen, const std::string& running) {
    const bool runningHasNotes = std::any_of(changelog.begin(), changelog.end(), [&](const ReleaseNotes& n) {
        return compareVersions(n.version, running) == 0;
    });
    if (!runningHasNotes) return {};
    std::vector<ReleaseNotes> out;
    for (const auto& n : changelog) {
        if (compareVersions(n.version, running) > 0) continue;
        const bool unseen = lastSeen.empty() ? compareVersions(n.version, running) == 0
                                             : compareVersions(n.version, lastSeen) > 0;
        if (unseen) out.push_back(n);
    }
    std::stable_sort(out.begin(), out.end(), [](const ReleaseNotes& a, const ReleaseNotes& b) {
        return compareVersions(a.version, b.version) > 0;
    });
    return out;
}

WhatsNewOffer whatsNewAtLaunch(const std::string& changelog, const std::string& lastSeen,
                               const std::string& running, bool firstBoot) {
    WhatsNewOffer offer;
    offer.remember = running;
    if (!firstBoot) offer.notes = notesBetween(readChangelog(changelog), lastSeen, running);
    return offer;
}

std::vector<ReleaseNotes> notesFor(const std::string& changelog, const std::string& version) {
    const auto all = readChangelog(changelog);
    auto mine = notesBetween(all, {}, version);
    if (mine.empty() && !all.empty()) mine.push_back(all.front());
    return mine;
}

std::string whatsNewMarkdown(const std::vector<ReleaseNotes>& notes) {
    std::string out;
    for (const auto& n : notes) {
        if (!out.empty()) out += "\n";
        out += "# " + n.version + "\n\n";
        if (!n.date.empty()) out += "> " + n.date + "\n\n";
        out += n.body;
    }
    return out;
}

}
