// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <initializer_list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <juce_core/juce_core.h>

#include "core/app/Catalogue.h"

namespace hum {

namespace i18n {

inline std::shared_ptr<const Catalogue>& installedCatalogue() {
    static std::shared_ptr<const Catalogue> held;
    return held;
}

inline std::shared_ptr<const Catalogue> current() {
    return std::atomic_load(&installedCatalogue());
}

inline void install(std::shared_ptr<const Catalogue> catalogue) {
    std::atomic_store(&installedCatalogue(), std::move(catalogue));
}

}

using TrArgs = std::initializer_list<std::pair<const char*, juce::String>>;

inline std::string trFilled(const std::string& text, TrArgs args, const std::pair<const char*, juce::String>* count = nullptr) {
    std::vector<std::pair<std::string, std::string>> values;
    if (count != nullptr) values.emplace_back(count->first, count->second.toStdString());
    for (const auto& [name, value] : args) values.emplace_back(name, value.toStdString());
    return fillPlaceholders(text, values);
}

inline juce::String trKey(const std::string& key, const juce::String& written) {
    if (const auto c = i18n::current())
        if (const auto* text = c->text(key)) return juce::String::fromUTF8(text->c_str());
    return written;
}

inline juce::String tr(const char* key, const char* written) {
    return trKey(key, juce::String::fromUTF8(written));
}

inline juce::String tr(const char* key, const char* written, TrArgs args) {
    return juce::String::fromUTF8(trFilled(tr(key, written).toStdString(), args).c_str());
}

inline juce::String trn(const char* key, const char* one, const char* other, long long n, TrArgs args = {}) {
    std::string text = n == 1 ? one : other;
    if (const auto c = i18n::current())
        if (const auto* found = c->plural(key, n)) text = *found;
    const std::pair<const char*, juce::String> count{"n", juce::String((juce::int64) n)};
    return juce::String::fromUTF8(trFilled(text, args, &count).c_str());
}

}
