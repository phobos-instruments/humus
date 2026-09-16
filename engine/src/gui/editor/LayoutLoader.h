// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include "hum/LayoutSpec.h"

namespace hum {

LayoutSpec loadLayoutSpec(const std::string& jsonText);
LayoutSpec loadLayoutSpecFromFile(const std::string& path);

bool knownLayoutControlType(const std::string& type);

}
