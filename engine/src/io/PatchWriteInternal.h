// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include "core/xml/Xml.h"

#include "io/PatchDocument.h"

namespace hum {

void addValue(xml::Element& prop, const Parameter& p);
void writeProperty(xml::Element& props, const Parameter& p);
void writePattern(xml::Element& propEl, const Pattern& pat);
void writePresets(xml::Element& presets, const OrganismModel& c);
void writeRollLocks(xml::Element& ce, const OrganismModel& c);
void writeRangeModes(xml::Element& ce, const OrganismModel& c);
void writeTrackInput(xml::Element& ce, const OrganismModel& c);
void writeTimelineRow(xml::Element& ce, const OrganismModel& c);
void writeAutomationLanes(xml::Element& mod, const OrganismModel& c);
void writeMidiSources(xml::Element& mod, const OrganismModel& c);
void writeOscSources(xml::Element& mod, const OrganismModel& c);
void writeModSources(xml::Element& mod, const OrganismModel& c);
void writeAutomationView(xml::Element& e, const AutomationView& v);
std::string pluginStateTag(const std::string& kind);
void writeMidiSettings(xml::Element& ce, const OrganismModel& c);
void writeOrganism(xml::Element& ce, const OrganismModel& c);
void writeConnection(xml::Element& e, const ConnectionModel& conn);
void writeView(xml::Element& ve, const OrganismView& v);
void writeMetapad(xml::Element& root, const MetapadModel& ms);

}
