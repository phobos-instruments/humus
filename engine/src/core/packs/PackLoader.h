// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <juce_core/juce_core.h>

namespace hum {

class PackLoader {
public:
    static PackLoader& instance();

    void loadInstalledPacks();

    bool loadPackDir(const std::string& dir, std::string& error);

    void unregisterPack(const std::string& id);

    bool installHumpack(const juce::File& bundle, std::string& error);
    static bool humpackEntriesStayInside(const juce::ZipFile& zip, std::string& offending);
    static bool shapeCompatible(uint64_t packShape, std::string& error);
    bool uninstallPack(const std::string& id, std::string& error);

    static juce::File userPacksDir();
    static void setUserPacksDirForTesting(const juce::File& dir);
    static std::string platformTag();

private:
    struct LoadedLib {
        std::string id;
        std::unique_ptr<juce::DynamicLibrary> lib;
    };
    std::vector<LoadedLib> libs_;
    bool scanned_ = false;
};

}
