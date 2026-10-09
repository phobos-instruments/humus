// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include "io/PatchDocument.h"

namespace hum {

std::vector<std::string> organismsWhoseStateChanged(const PatchDocumentModel& before, const PatchDocumentModel& after);

}
