// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

namespace hum {

struct ReleaseNotes {
    std::string version;
    std::string date;
    std::string body;
};

std::vector<ReleaseNotes> readChangelog(const std::string& markdown);

std::vector<ReleaseNotes> notesBetween(const std::vector<ReleaseNotes>& changelog,
                                       const std::string& lastSeen, const std::string& running);

struct WhatsNewOffer {
    std::vector<ReleaseNotes> notes;
    std::string remember;
};

WhatsNewOffer whatsNewAtLaunch(const std::string& changelog, const std::string& lastSeen,
                               const std::string& running, bool firstBoot);

std::vector<ReleaseNotes> notesFor(const std::string& changelog, const std::string& version);

std::string whatsNewMarkdown(const std::vector<ReleaseNotes>& notes);

}
