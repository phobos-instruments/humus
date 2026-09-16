// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <map>
#include <string>

#include <juce_audio_processors/juce_audio_processors.h>

namespace hum {

inline std::map<const juce::AudioProcessorParameter*, std::string>
pluginTreeGroups(juce::AudioProcessor& inst) {
    std::map<const juce::AudioProcessorParameter*, std::string> out;
    std::function<void(const juce::AudioProcessorParameterGroup&, const std::string&)> walk =
        [&](const juce::AudioProcessorParameterGroup& g, const std::string& top) {
            for (auto* node : g) {
                if (auto* sub = node->getGroup())
                    walk(*sub, top.empty() ? sub->getName().toStdString() : top);
                else if (auto* pp = node->getParameter(); pp && !top.empty())
                    out[pp] = top;
            }
        };
    walk(inst.getParameterTree(), {});
    return out;
}

}
