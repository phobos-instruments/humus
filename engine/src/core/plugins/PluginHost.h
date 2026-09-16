// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

#include "core/params/ParamSchema.h"
#include "hum/Organism.h"

namespace hum {

class PluginHost {
public:
    static PluginHost& instance();

    juce::AudioPluginFormatManager& formats() { return formats_; }
    juce::KnownPluginList& knownPlugins() { return list_; }

    static std::string classRawFor(const juce::PluginDescription& desc);
    std::unique_ptr<juce::PluginDescription> descriptionFor(const std::string& classRaw) const;

    std::unique_ptr<juce::AudioPluginInstance> createInstance(const juce::PluginDescription& desc,
                                                              double sampleRate, int blockSize,
                                                              std::string& error);

    static OrganismPtr createOrganism(const std::string& classRaw);

    std::vector<std::string> classRawList() const;
    static std::vector<std::string> allPaletteClasses();
    bool isPluginClass(const std::string& classRaw) const;
    bool isInstrument(const std::string& classRaw);
    std::string vendorOf(const std::string& classRaw);

    const std::vector<ParamDesc>& schemaFor(const std::string& classRaw);
    static constexpr int kMaxParams = 256;

    struct PluginIoFacts {
        int ins = 2, outs = 2;
        bool acceptsMidi = false, producesMidi = false, hasEditor = true;
    };
    const PluginIoFacts& ioFactsFor(const std::string& classRaw);

    std::string knownListToXml() const;
    void restoreKnownListFromXml(const std::string& xml);

    void setBridgeExe(const juce::File& exe) { bridgeExe_ = exe; }
    const juce::File& bridgeExe() const { return bridgeExe_; }
    void setBridgePolicy(std::function<bool(const std::string& classRaw)> p) {
        bridgePolicy_ = std::move(p);
    }
    bool shouldBridge(const std::string& classRaw) const {
        return bridgeExe_ != juce::File() && bridgePolicy_ && bridgePolicy_(classRaw);
    }

private:
    PluginHost();

    struct PaletteFacts { std::string vendor; bool instrument = false; };
    const PaletteFacts* paletteFactsFor(const std::string& classRaw);
public:
    void invalidatePaletteFacts() { factsCache_.clear(); factsCount_ = -1; }

private:
    juce::AudioPluginFormatManager formats_;
    juce::KnownPluginList list_;
    std::unordered_map<std::string, PaletteFacts> factsCache_;
    int factsCount_ = -1;
    std::unordered_map<std::string, std::vector<ParamDesc>> schemaCache_;
    std::unordered_map<std::string, PluginIoFacts> ioCache_;
    juce::File bridgeExe_;
    std::function<bool(const std::string&)> bridgePolicy_;
};

}
