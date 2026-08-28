#pragma once
#include <string>

#include <juce_core/juce_core.h>

namespace hum {

namespace pluginListStore {

std::string load();

void save(const std::string& xml);

std::string loadFrom(const juce::File& appDir);
void saveTo(const juce::File& appDir, const std::string& xml);

}
}
