#pragma once
#include <juce_core/juce_core.h>

#include "io/PatchDocument.h"

namespace hum {

void parsePattern(juce::XmlElement& pe, Pattern& pat);

void parseModulationSources(juce::XmlElement& mod, OrganismModel& c);

void parseMidiSettings(juce::XmlElement& organismEl, OrganismModel& c);

}
