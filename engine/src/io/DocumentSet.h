#pragma once
#include <string>
#include <vector>

#include <juce_core/juce_core.h>

#include "io/PatchFormat.h"

namespace hum {

inline constexpr int kDocumentSetMaxSlots = 128;
inline constexpr const char* kDocumentSetExtension = ".hums";
inline constexpr const char* kDocumentSetFilter = "*.hums";

inline juce::String serializeDocumentSet(const std::vector<juce::String>& paths,
                                         const juce::File& setFile) {
    juce::XmlElement root("humus-document-set");
    root.setAttribute("version", 1);
    const juce::File base = setFile.getParentDirectory();
    int n = 0;
    for (const auto& p : paths) {
        if (n++ >= kDocumentSetMaxSlots) break;
        const juce::File f(p);
        auto* d = root.createNewChildElement("document");
        const bool rel = base != juce::File() && f.isAChildOf(base);
        d->setAttribute("path", rel ? toDocumentPath(f.getRelativePathFrom(base)) : p);
    }
    return root.toString();
}

inline std::vector<juce::String> parseDocumentSet(const juce::String& xml,
                                                  const juce::File& setFile) {
    std::vector<juce::String> out;
    const auto root = juce::XmlDocument::parse(xml);
    if (root == nullptr || !root->hasTagName("humus-document-set")) return out;
    for (auto* d : root->getChildWithTagNameIterator("document")) {
        if ((int) out.size() >= kDocumentSetMaxSlots) break;
        const auto p = d->getStringAttribute("path");
        if (p.isEmpty()) continue;
        if (juce::File::isAbsolutePath(p))
            out.push_back(p);
        else
            out.push_back(setFile.getParentDirectory()
                              .getChildFile(fromDocumentPath(p))
                              .getFullPathName());
    }
    return out;
}

}
