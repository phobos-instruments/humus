// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <cstdint>

namespace hum::phbank {

int sixOpFactoryCount();
void fillSixOpFactory(uint8_t bank[][128], int slots);

}
