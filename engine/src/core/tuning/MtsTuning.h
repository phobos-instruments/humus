// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>

#include "hum/Tuning.h"

namespace hum {

std::vector<juce::MidiMessage> mtsRetuneMessages(const Tuning& t);

void mtsNoteTriple(double hz, unsigned char out[3]);

}
