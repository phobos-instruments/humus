// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include <juce_core/juce_core.h>

#include "core/params/UnitText.h"

namespace hum {

juce::String unitFormat(Unit u, double value, double min, double max);
double unitParse(Unit u, const juce::String& text, double min, double max);

juce::String unitPlain(Unit u, double value, double min, double max);
double unitPlainParse(Unit u, const juce::String& text, double min, double max);

bool isRelativeEntry(const juce::String& text);
double applyRelativeEntry(const juce::String& text, double current);

}
