#pragma once
#include <string>
#include <utility>
#include <vector>

#include <juce_core/juce_core.h>

#include "core/ParamSchema.h"

namespace hum {

juce::String genieSystemPrompt(const std::string& className,
                               const std::vector<ParamDesc>& schema);

std::vector<std::pair<std::string, double>> parseGenieReply(
    const juce::String& reply, const std::vector<ParamDesc>& schema);

}
