// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/packs/DesktopPacks.h"

#include <juce_core/juce_core.h>

#include "core/app/AppPaths.h"
#include "core/packs/BuiltinPacks.h"
#include "core/packs/PackRegistry.h"
#include "core/packs/PackRoots.h"
#include "core/plugins/HostedPlugins.h"
#include "core/plugins/PluginHost.h"

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
        for (const auto& c : bundleCandidates(binaryDirs(), "packs"))
            if (holdsPacks(c)) return c.getFullPathName().toStdString();
        if (const auto repo = sourceTreePacks(); holdsPacks(repo)) return repo.getFullPathName().toStdString();
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
