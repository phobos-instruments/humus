// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/assistant/AssistantEngine.h"

namespace hum {

AssistantEngine::AssistantEngine(AssistantHost& host)
    : host_(host) {
    messages_ = juce::Array<juce::var>();
}

AssistantEngine::~AssistantEngine() {
    *alive_ = false;
    if (busy_) host_.endTransaction();
}

void AssistantEngine::runToolForTest(const juce::String& name, std::function<void(juce::String)> done) {
    ToolCall c;
    c.name = name;
    runToolAsync(c, std::move(done));
}

void AssistantEngine::send(const juce::String& userText) {
    if (busy_ || userText.trim().isEmpty()) return;
    line("you", userText);
    messages_.append(userMessage(userText));
    rounds_ = 0;
    edited_ = false;
    nudged_ = false;
    setBusy(true);
    host_.beginTransaction();
    continueLoop();
}

void AssistantEngine::line(const juce::String& role, const juce::String& text) {
    if (onLine && text.isNotEmpty()) onLine(role, text);
}

void AssistantEngine::setBusy(bool b) {
    busy_ = b;
    if (onBusy) onBusy(b);
}

void AssistantEngine::finishTurn() {
    host_.endTransaction();
    setBusy(false);
    if (edited_ && onPatchEdited) onPatchEdited();
}

void AssistantEngine::continueLoop() {
    AiClient::ChatRequest req;
    req.system = assistantSystemPrompt();
    req.messages = messages_;
    req.tools = assistantToolsSpec();
    AiClient::chat(std::move(req),
                       [this, alive = alive_](juce::var response, juce::String error) {
        if (!*alive) return;
        if (error.isNotEmpty()) { line("error", error); finishTurn(); return; }
        messages_.append(assistantMessage(response));
        line("assistant", extractText(response));

        const auto calls = extractToolCalls(response);
        if (calls.empty() || !wantsTools(response)) {
            const auto text = extractText(response);
            const bool phantomProse = !edited_ && claimsEdits(text);
            if (!nudged_ && (phantomProse || looksLikePhantomEdits(text))) {
                nudged_ = true;
                line("error", phantomProse
                    ? "That reply claims edits, but no editing tool ran - "
                      "nothing changed. Asking the model to do it for real."
                    : "That reply only pretended to edit (code instead of "
                      "tool calls) - asking the model to do it for real.");
                messages_.append(userMessage(phantomProse ? editClaimNudge()
                                                          : toolNudge()));
                continueLoop();
                return;
            }
            if (phantomProse)
                line("error", tr("assistant-engine.heads-up-nothing-was-actually", "Heads up: nothing was actually changed this turn."));
            finishTurn();
            return;
        }
        if (++rounds_ > kMaxRounds) {
            line("error", "Stopped: too many tool rounds for one request.");
            finishTurn();
            return;
        }
        runToolChain(std::make_shared<std::vector<ToolCall>>(calls), 0,
                     std::make_shared<std::vector<std::pair<juce::String, juce::String>>>());
    });
}

void AssistantEngine::runToolChain(std::shared_ptr<std::vector<ToolCall>> calls, size_t i, std::shared_ptr<std::vector<std::pair<juce::String, juce::String>>> results) {
    if (i >= calls->size()) {
        messages_.append(toolResultsMessage(*results));
        continueLoop();
        return;
    }
    const ToolCall& c = (*calls)[i];
    line("tool", progressLine(c));
    runToolAsync(c, [this, alive = alive_, calls, i, results, id = c.id](juce::String result) {
        if (!*alive) return;
        if (result.startsWith("error")) line("error", result);
        results->emplace_back(id, result);
        runToolChain(calls, i + 1, results);
    });
}

void AssistantEngine::runToolAsync(const ToolCall& c, std::function<void(juce::String)> done) {
    if (c.name == "levels") {
        if (!host_.isPlaying()) { done(kNotPlaying); return; }
        measurePeaksAsync([this, done](const std::map<std::string, float>& peaks) {
            done(levelsJson(peaks));
        });
        return;
    }
    if (c.name == "spectrum") {
        if (!host_.isPlaying()) { done(kNotPlaying); return; }
        const double sr = host_.sampleRate();
        if (!host_.armNodeCapture((int) (sr * 0.7))) { done("error: no live audio engine"); return; }
        ticker_.start(900, 1, {}, [this, done, sr] {
            host_.disarmNodeCapture();
            done(spectrumJsonFromCapture(sr));
        });
        return;
    }
    if (c.name == "automix") {
        if (!host_.isPlaying()) { done(kNotPlaying); return; }
        measurePeaksAsync([this, done](const std::map<std::string, float>& first) {
            auto staged = std::make_shared<AutomixStage>(automixStage(first));
            if (staged->error.isNotEmpty()) { done(staged->error); return; }
            measurePeaksAsync([this, done, staged](const std::map<std::string, float>& second) {
                done(automixFinish(*staged, second));
            });
        });
        return;
    }
    done(runTool(c));
}

juce::String AssistantEngine::unknownNode(const juce::var& n) {
    return "error: no node named '" + n.toString() + "' (see list_patch)";
}

std::string AssistantEngine::resolveClass(const std::string& cls) {
    std::string ci;
    for (const auto& n : Registry::instance().classNames()) {
        if (n == cls) return n;
        if (ci.empty() && juce::String(n).equalsIgnoreCase(juce::String(cls))) ci = n;
    }
    return ci;
}

juce::String AssistantEngine::unknownClass(const std::string& cls) {
    juce::StringArray close;
    const auto needle = juce::String(cls).toLowerCase();
    for (const auto& n : Registry::instance().classNames()) {
        const auto hay = juce::String(n).toLowerCase();
        if (hay.contains(needle) || needle.contains(hay)) close.add(n);
        if (close.size() >= 5) break;
    }
    return "error: unknown class '" + juce::String(cls) + "'"
           + (close.isEmpty() ? juce::String(" (see list_classes)")
                              : " - closest classes: " + close.joinIntoString(", "));
}

bool AssistantEngine::fileTextAllowed(const std::string& text) {
    if (text.empty()) return true;
    const auto colon = text.find(':');
    const bool isRef = colon != std::string::npos && colon > 0 && colon + 1 < text.size()
                       && text[colon + 1] != '/' && text[colon + 1] != '\\';
    if (isRef) return true;
    const juce::String path(juce::CharPointer_UTF8(text.c_str()));
    if (!juce::File::isAbsolutePath(path)) return true;
    const juce::File f(path);
    const auto home = juce::File::getSpecialLocation(juce::File::userHomeDirectory);
    return f.isAChildOf(home) || f.isAChildOf(appDataDir());
}

}
