// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <memory>
#include <set>

#include <juce_core/juce_core.h>

namespace hum {

class AppSettings {
public:
    static AppSettings& instance();

    bool existedAtBoot() const { return existedAtBoot_; }

    juce::String getString(const juce::String& key, const juce::String& def = {}) const;
    double getDouble(const juce::String& key, double def) const;
    int getInt(const juce::String& key, int def) const;

    void set(const juce::String& key, const juce::String& value);
    void set(const juce::String& key, double value);
    void set(const juce::String& key, int value);

    void beginBatch() { ++batch_; }
    void endBatch();

    static juce::File& fileForTesting() {
        static juce::File f;
        return f;
    }
    static juce::File file();

private:
    AppSettings();
    void save();

    std::unique_ptr<juce::XmlElement> root_;
    bool existedAtBoot_ = false;
    std::set<juce::String> dirtyKeys_;
    int batch_ = 0;
    bool dirty_ = false;
};

}
