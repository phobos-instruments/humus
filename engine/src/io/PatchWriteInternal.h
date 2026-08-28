#pragma once
#include <juce_core/juce_core.h>

#include "io/PatchDocument.h"

namespace hum {

void addValue(juce::XmlElement& prop, const Parameter& p);
void writeProperty(juce::XmlElement& props, const Parameter& p);
void writePattern(juce::XmlElement& propEl, const Pattern& pat);
void writePresets(juce::XmlElement& presets, const OrganismModel& c);
void writeRollLocks(juce::XmlElement& ce, const OrganismModel& c);
void writeAutomationLanes(juce::XmlElement& mod, const OrganismModel& c);
void writeMidiSources(juce::XmlElement& mod, const OrganismModel& c);
void writeOscSources(juce::XmlElement& mod, const OrganismModel& c);
void writeModSources(juce::XmlElement& mod, const OrganismModel& c);
void writeAutomationView(juce::XmlElement& e, const AutomationView& v);
juce::String pluginStateTag(const std::string& kind);
void writeMidiSettings(juce::XmlElement& ce, const OrganismModel& c);
void writeOrganism(juce::XmlElement& ce, const OrganismModel& c);
void writeConnection(juce::XmlElement& e, const ConnectionModel& conn);
void writeView(juce::XmlElement& ve, const OrganismView& v);
void writeMetapad(juce::XmlElement& root, const MetapadModel& ms);

}
