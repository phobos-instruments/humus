// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include "core/net/ControlShape.h"

namespace hum {

class ModelHost;

double currentParamValue(ModelHost& host, const std::string& organism, const std::string& param);
void applyControl(ModelHost& host, const ControlUpdate& u);

}
