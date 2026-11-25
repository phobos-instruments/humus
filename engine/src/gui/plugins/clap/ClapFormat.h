// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace hum::claphost {

class Format : public juce::AudioPluginFormat {
public:
    static constexpr const char* kName = "CLAP";

    juce::String getName() const override { return kName; }
    void findAllTypesForFile(juce::OwnedArray<juce::PluginDescription>& results, const juce::String& file) override;
    bool fileMightContainThisPluginType(const juce::String& file) override;
    juce::String getNameOfPluginFromIdentifier(const juce::String& file) override;
    bool pluginNeedsRescanning(const juce::PluginDescription& description) override;
    bool doesPluginStillExist(const juce::PluginDescription& description) override;
    bool canScanForPlugins() const override { return true; }
    bool isTrivialToScan() const override { return false; }
    juce::StringArray searchPathsForPlugins(const juce::FileSearchPath& directories, bool recursive,
                                            bool allowAsync) override;
    juce::FileSearchPath getDefaultLocationsToSearch() override;
    bool requiresUnblockedMessageThreadDuringCreation(const juce::PluginDescription&) const override { return false; }

private:
    void createPluginInstance(const juce::PluginDescription& description, double sampleRate, int blockSize,
                              PluginCreationCallback callback) override;
};

void addClapFormat(juce::AudioPluginFormatManager& formats);
void enableClapHosting();

}
