// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <utility>
#include <vector>

#include <juce_core/juce_core.h>

#include "core/params/ParamSchema.h"

namespace hum {

juce::String genieSystemPrompt(const std::string& className,
                               const std::vector<ParamDesc>& schema);

std::vector<std::pair<std::string, double>> parseGenieReply(
    const juce::String& reply, const std::vector<ParamDesc>& schema);

}
