// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <vector>

#include "Ph/PhOpl.h"

namespace hum::phbank {
const std::vector<PhOpl::Instrument>& oplFactory();
}
