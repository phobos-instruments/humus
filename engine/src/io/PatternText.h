// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include "hum/Pattern.h"

namespace hum {

std::string patternToText(const Pattern& pattern);
bool patternFromText(const std::string& text, Pattern& pattern);

}
