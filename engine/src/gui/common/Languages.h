// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <memory>
#include <string>

#include <BinaryData.h>
#include <juce_core/juce_core.h>

#include "core/app/AppPaths.h"
#include "core/app/Catalogue.h"
#include "core/app/LanguageCatalogue.h"
#include "core/app/PageLanguage.h"
#include "gui/app/AppSettings.h"
#include "gui/common/Localisation.h"

namespace hum::i18n {

struct Language {
    juce::String code, name;
};

inline constexpr const char* kSystem = "system";

inline std::string embeddedInterface(const std::string& code) {
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i) {
        const auto* res = BinaryData::namedResourceList[i];
        if (interfaceCodeOf(BinaryData::getNamedResourceOriginalFilename(res)) != code) continue;
        int size = 0;
        if (const char* data = BinaryData::getNamedResource(res, size); data != nullptr && size > 0)
            return std::string(data, (size_t) size);
    }
    return {};
}

inline std::string interfaceText(const juce::String& code) {
    for (const auto& dir : assetSearchPath("Translations"))
        if (const auto f = dir.getChildFile(code + ".po"); f.existsAsFile())
            return f.loadFileAsString().toStdString();
    return embeddedInterface(code.toStdString());
}

inline juce::StringArray interfaceCodes() {
    juce::StringArray codes;
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
        if (const auto code = interfaceCodeOf(BinaryData::getNamedResourceOriginalFilename(BinaryData::namedResourceList[i]));
            !code.empty())
            codes.addIfNotAlreadyThere(juce::String(code));
    for (const auto& dir : assetSearchPath("Translations"))
        for (const auto& f : dir.findChildFiles(juce::File::findFiles, false, "*.po"))
            codes.addIfNotAlreadyThere(f.getFileNameWithoutExtension());
    return codes;
}

inline juce::Array<Language> available() {
    std::vector<std::pair<std::string, std::string>> interfaces;
    for (const auto& code : interfaceCodes()) interfaces.emplace_back(code.toStdString(), interfaceText(code));
    juce::Array<Language> out;
    for (const auto& choice : languageChoices(interfaces))
        out.add({juce::String(choice.code), juce::String::fromUTF8(choice.name.c_str())});
    return out;
}

inline Catalogue catalogueFor(const juce::String& code) {
    auto catalogue = Catalogue::fromPo(interfaceText(code));
    addPackStrings(catalogue, loadedPackDirs(), code.toStdString());
    return catalogue;
}

inline juce::String& appliedCode() {
    static juce::String code;
    return code;
}

inline bool apply(const juce::String code) {
    pageLanguage().clear();
    appliedCode() = {};
    install(nullptr);
    if (code.isEmpty() || code == kSystem) return true;
    if (interfaceText(code).empty()) return false;
    install(std::make_shared<const Catalogue>(catalogueFor(code)));
    pageLanguage() = code.toStdString();
    appliedCode() = code;
    return true;
}

inline void refresh() {
    const auto code = appliedCode();
    if (code.isNotEmpty()) apply(code);
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
