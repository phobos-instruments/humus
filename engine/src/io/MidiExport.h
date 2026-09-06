#pragma once
#include <string>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>

#include "io/PatchDocument.h"

namespace hum::midiexport {

struct Range {
    double fromBeat = 0.0;
    double toBeat = 0.0;
};

std::vector<std::string> notedNodes(const PatchDocumentModel& model);
bool write(const PatchDocumentModel& model, const juce::File& out, const Range& range);

}
