// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "core/xml/Xml.h"

#include "io/PatchDocument.h"

namespace hum {

void parsePattern(xml::Element& pe, Pattern& pat);

void parseModulationSources(xml::Element& mod, OrganismModel& c);

void parseMidiSettings(xml::Element& organismEl, OrganismModel& c);

}
