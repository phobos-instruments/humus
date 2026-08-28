#pragma once
#include <string>

#include <juce_core/juce_core.h>

#include "core/AppPaths.h"

namespace hum::library {

inline constexpr const char* kRefPrefix = "library:";

inline std::string& configuredRoot() { return configuredContentRoot(); }
inline juce::File root() { return userContentRoot(); }

inline bool isRef(const std::string& text) { return text.rfind(kRefPrefix, 0) == 0; }

inline std::string resolve(const std::string& text) {
    if (!isRef(text)) return text;
    const juce::String rel(juce::CharPointer_UTF8(
        text.substr(std::string(kRefPrefix).size()).c_str()));
    return root().getChildFile(rel).getFullPathName().toStdString();
}

inline std::string referenceFor(const std::string& text) {
    if (text.empty() || isRef(text)) return text;
    const juce::String s(juce::CharPointer_UTF8(text.c_str()));
    if (!juce::File::isAbsolutePath(s)) return text;
    const juce::File f(s);
    if (!f.isAChildOf(root())) return text;
    return kRefPrefix
         + f.getRelativePathFrom(root()).replaceCharacter('\\', '/').toStdString();
}

}
