// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <vector>

namespace hum {

bool bodyCoreMLReady();

bool bodyCoreMLPredict(bool landmarkNet, const float* in, int count,
                       std::vector<float>& out0, std::vector<float>& out1);

}
