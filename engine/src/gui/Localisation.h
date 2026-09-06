#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "core/AppPaths.h"
#include "gui/AppSettings.h"

namespace hum {

inline juce::String tr(const char* key, const char* written) {
    return juce::translate(juce::String(key), juce::String(written));
}

}

namespace hum::i18n {

struct Language {
    juce::String code, name;
    juce::File file;
};

inline constexpr const char* kSystem = "system";
inline constexpr const char* kTemplateName = "template";

inline juce::Array<Language> available() {
    juce::Array<Language> out;
    juce::StringArray seen;
    for (const auto& dir : assetSearchPath("Translations"))
        for (const auto& f : dir.findChildFiles(juce::File::findFiles, false, "*.txt")) {
            const auto code = f.getFileNameWithoutExtension();
            if (code == kTemplateName || seen.contains(code)) continue;
            seen.add(code);
            const juce::LocalisedStrings parsed(f, true);
            out.add({code, parsed.getLanguageName().isNotEmpty() ? parsed.getLanguageName() : code,
                     f});
        }
    return out;
}

inline bool apply(const juce::String& code) {
    if (code.isEmpty() || code == kSystem) {
        juce::LocalisedStrings::setCurrentMappings(nullptr);
        return true;
    }
    for (const auto& lang : available())
        if (lang.code == code) {
            juce::LocalisedStrings::setCurrentMappings(new juce::LocalisedStrings(lang.file, true));
            return true;
        }
    juce::LocalisedStrings::setCurrentMappings(nullptr);
    return false;
}

inline juce::String saved() {
    return AppSettings::instance().getString("ui.language", kSystem);
}

inline void applySaved() { apply(saved()); }

inline void choose(const juce::String& code) {
    AppSettings::instance().set("ui.language", code);
    apply(code);
}

}
