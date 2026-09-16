// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include <juce_core/juce_core.h>

namespace hum {

struct ToolCall {
    juce::String id;
    juce::String name;
    juce::var input;
};

juce::var assistantToolsSpec();
juce::String assistantSystemPrompt();
juce::String progressLine(const ToolCall& call);

bool looksLikePhantomEdits(const juce::String& text);
juce::String toolNudge();
bool claimsEdits(const juce::String& text);
juce::String editClaimNudge();

juce::String extractText(const juce::var& response);
std::vector<ToolCall> extractToolCalls(const juce::var& response);
bool wantsTools(const juce::var& response);

juce::var userMessage(const juce::String& text);
juce::var assistantMessage(const juce::var& response);
juce::var toolResultsMessage(const std::vector<std::pair<juce::String, juce::String>>&
                                 idAndContent);

}
