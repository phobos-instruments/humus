// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/packs/DesktopPacks.h"

#include <juce_core/juce_core.h>

#include "core/packs/BuiltinPacks.h"
#include "core/packs/PackRegistry.h"
#include "core/packs/PackRoots.h"
#include "core/plugins/HostedPlugins.h"
#include "core/plugins/PluginHost.h"

#ifndef HUM_PACKS_DIR
#define HUM_PACKS_DIR ""
#endif

namespace hum {

namespace {

class DesktopPackRoots : public PackRoots {
public:
    std::string builtinRoot() const override {
        auto holdsPacks = [](const juce::File& d) {
            if (!d.isDirectory()) return false;
            for (const auto& c : d.findChildFiles(juce::File::findDirectories, false))
                if (c.getChildFile("pack.json").existsAsFile()) return true;
            return false;
        };
        const auto exeDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();
        for (const auto& c : {exeDir.getParentDirectory().getChildFile("Resources").getChildFile("packs"),
                              exeDir.getChildFile("packs"), exeDir.getParentDirectory().getChildFile("packs")})
            if (holdsPacks(c)) return c.getFullPathName().toStdString();
        juce::File repo(juce::String(HUM_PACKS_DIR));
        if (holdsPacks(repo)) return repo.getFullPathName().toStdString();
        return {};
    }

    std::vector<std::string> builtinIds() const override {
        std::vector<std::string> ids;
        for (const auto& pack : builtinPacks()) ids.emplace_back(pack.id);
        return ids;
    }
};

class DesktopHostedPlugins : public HostedPlugins {
public:
    const std::vector<ParamDesc>& schemaFor(const std::string& className) const override {
        return PluginHost::instance().schemaFor(className);
    }
    bool isInstrument(const std::string& className) const override {
        return PluginHost::instance().isInstrument(className);
    }
    std::string vendorOf(const std::string& className) const override {
        return PluginHost::instance().vendorOf(className);
    }
};

}

void installDesktopPacks() {
    static const DesktopPackRoots roots;
    static const DesktopHostedPlugins plugins;
    PackRegistry::instance().setRoots(&roots);
    setHostedPlugins(&plugins);
}

}
