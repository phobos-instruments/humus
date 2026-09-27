// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include <juce_core/juce_core.h>

#include "core/xml/Xml.h"
#include "io/PatchFormat.h"

namespace hum {

inline constexpr int kDocumentSetMaxSlots = 128;
inline constexpr const char* kDocumentSetExtension = ".hums";
inline constexpr const char* kDocumentSetFilter = "*.hums";

inline juce::String serializeDocumentSet(const std::vector<juce::String>& paths,
                                         const juce::File& setFile) {
    xml::Element root("humus-document-set");
    root.setAttribute("version", 1);
    const juce::File base = setFile.getParentDirectory();
    int n = 0;
    for (const auto& p : paths) {
        if (n++ >= kDocumentSetMaxSlots) break;
        const juce::File f(p);
        auto* d = root.addChild("document");
        const bool rel = base != juce::File() && f.isAChildOf(base);
        d->setAttribute("path", (rel ? toDocumentPath(f.getRelativePathFrom(base)) : p).toStdString());
    }
    return juce::String::fromUTF8(xml::write(root).c_str());
}

inline std::vector<juce::String> parseDocumentSet(const juce::String& xml,
                                                  const juce::File& setFile) {
    std::vector<juce::String> out;
    const auto root = xml::parse(xml.toStdString());
    if (root == nullptr || !root->hasTag("humus-document-set")) return out;
    for (auto* d : root->children()) {
        if (!d->hasTag("document")) continue;
        if ((int) out.size() >= kDocumentSetMaxSlots) break;
        const auto p = juce::String::fromUTF8(d->attribute("path").c_str());
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
