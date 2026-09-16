// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/app/AppCommandLine.h"

#include <iostream>

#include "core/plugins/PluginHost.h"
#include "core/plugins/PluginListStore.h"
#include "core/plugins/PluginScanner.h"
#include "core/tuning/TuningProbe.h"
#include "gui/assistant/AiClient.h"
#include "hum/Registry.h"

namespace hum::appcli {

namespace {

Outcome aiPing(const juce::StringArray& args, int at) {
    int status = 0;
    const auto base = at + 1 < args.size() && args[at + 1].startsWith("http")
                          ? args[at + 1] : AiClient::endpoint();
    auto stream = juce::URL(base + "/api/version").createInputStream(
        juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
            .withConnectionTimeoutMs(5000)
            .withStatusCode(&status));
    std::cout << (stream != nullptr
                      ? "OK " + stream->readEntireStreamAsString()
                      : "FAIL (no connection, status " + juce::String(status) + ")")
              << " - " << base << std::endl;
    return {true, stream != nullptr ? 0 : 1};
}

Outcome tuningProbe(const juce::StringArray& args, int at) {
    hum::registerBuiltinOrganisms();
    hum::PluginHost::instance().restoreKnownListFromXml(hum::pluginListStore::load());
    const auto verdict = hum::probeTuning(args[at + 2].toStdString());
    juce::File(args[at + 1]).replaceWithText(hum::tuningProbeVerdictName(verdict));
    return {true, 0};
}

}

Outcome run(const juce::StringArray& args) {
    if (const int at = args.indexOf("--ai-ping"); at >= 0) return aiPing(args, at);

    if (const int at = args.indexOf("--tuning-probe"); at >= 0 && at + 2 < args.size())
        return tuningProbe(args, at);

    if (const int at = args.indexOf("--scan-enumerate-out"); at >= 0 && at + 2 < args.size()) {
        juce::File(args[at + 1]).replaceWithText(PluginScanner::enumerateLines(args[at + 2]));
        return {true, 0};
    }

    if (const int at = args.indexOf("--scan-plugin-out"); at >= 0 && at + 2 < args.size()) {
        const juce::File out(args[at + 1]);
        const juce::String fmt = args[at + 2];
        juce::StringArray files;
        for (int i = at + 3; i < args.size(); ++i) files.add(args[i]);
        out.replaceWithText(PluginScanner::probeFilesXml(fmt, files));
        return {true, 0};
    }

    if (const int at = args.indexOf("--scan-plugin"); at >= 0) {
        const juce::String fmt = at + 1 < args.size() ? args[at + 1] : juce::String();
        juce::StringArray files;
        for (int i = at + 2; i < args.size(); ++i) files.add(args[i]);
        std::cout << PluginScanner::probeFilesXml(fmt, files) << std::endl;
        return {true, 0};
    }

    return {};
}

}
