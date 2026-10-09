// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

namespace hum {

std::string& pageLanguage();

std::vector<std::string> pageCandidates(const std::string& path);

std::vector<std::string> embeddedCandidates(const std::string& stem);

std::string withoutFrontMatter(const std::string& page);

}
