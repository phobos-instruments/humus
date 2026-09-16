// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include "core/net/ControlShape.h"
#include "hum/caps/Graph.h"

namespace hum {

struct ModRoute {
    int srcNode = -1;
    std::string srcValue;
    bool srcIsParam = false;
    int srcSlot = -1;
    double srcMin = 0.0;
    double srcMax = 1.0;
    int dstNode = -1;
    std::string dstParam;
    int dstSlot = -1;
    double min = 0.0;
    double max = 1.0;
    bool carry = false;
    double dstLo = 0.0;
    double dstHi = 0.0;
    ControlShape shape;
    ControlShapeState state;
    const ControlSource* src = nullptr;
};

}
