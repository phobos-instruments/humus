// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstddef>
#include <string>
#include <vector>

#include "io/PatchDocument.h"

namespace hum::history {

struct Change {
    enum class Kind { Added, Removed, CordsAdded, CordsRemoved, Tempo, Pattern, Automation, Setting };
    Kind kind = Kind::Setting;
    std::string organism;
    std::string detail;
    int count = 0;
};

std::vector<Change> changesBetween(const PatchDocumentModel& before, const PatchDocumentModel& after);

std::string phraseOf(const Change& change);

std::string summaryOf(const std::vector<Change>& changes, std::size_t shown = 3);

}
