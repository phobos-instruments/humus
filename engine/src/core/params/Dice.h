// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

namespace hum {

class Dice {
public:
    virtual ~Dice() = default;
    virtual double nextDouble() = 0;
    virtual int nextInt(int below) = 0;
};

}
