#pragma once
#include <string>

#include "io/PatchDocument.h"

namespace juce { class XmlElement; }

namespace hum {

bool writePatchFile(const std::string& path, const PatchDocumentModel& doc, std::string& error,
                  const juce::XmlElement* original = nullptr);

std::string writePatchString(const PatchDocumentModel& doc,
                             const juce::XmlElement* original = nullptr);

}
