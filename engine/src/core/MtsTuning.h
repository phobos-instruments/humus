#pragma once
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>

#include "hum/Tuning.h"

namespace hum {

std::vector<juce::MidiMessage> mtsRetuneMessages(const Tuning& t);

void mtsNoteTriple(double hz, unsigned char out[3]);

}
