// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <utility>
#include <vector>

#include "core/app/Catalogue.h"

namespace hum {

struct LanguageChoice {
    std::string code, name;
};

std::string interfaceCodeOf(const std::string& embeddedName);

std::vector<LanguageChoice> languageChoices(const std::vector<std::pair<std::string, std::string>>& interfaces);

std::string packStringsPath(const std::string& packDir, const std::string& code);

void addPackStrings(Catalogue& into, const std::vector<std::string>& packDirs, const std::string& code);

std::vector<std::string> loadedPackDirs();

}
