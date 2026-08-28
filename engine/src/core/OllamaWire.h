#pragma once
#include <juce_core/juce_core.h>

namespace hum {

juce::var ollamaMessagesFromAnthropic(const juce::String& system, const juce::var& messages);

juce::var ollamaToolsFromAnthropic(const juce::var& tools);

juce::var anthropicResponseFromOllama(const juce::var& ollamaResponse);

}
