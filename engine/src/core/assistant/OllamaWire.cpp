// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/assistant/OllamaWire.h"

namespace hum {

namespace {
juce::var obj() { return juce::var(new juce::DynamicObject()); }
void set(juce::var& v, const char* key, const juce::var& value) {
    v.getDynamicObject()->setProperty(key, value);
}
}

juce::var ollamaMessagesFromAnthropic(const juce::String& system, const juce::var& messages) {
    juce::Array<juce::var> out;
    if (system.isNotEmpty()) {
        auto m = obj();
        set(m, "role", "system");
        set(m, "content", system);
        out.add(m);
    }
    if (const auto* arr = messages.getArray())
        for (const auto& m : *arr) {
            const auto role = m["role"].toString();
            const auto content = m["content"];
            if (!content.isArray()) {
                auto o = obj();
                set(o, "role", role);
                set(o, "content", content.toString());
                out.add(o);
                continue;
            }
            if (role == "assistant") {
                juce::String text;
                juce::Array<juce::var> calls;
                for (const auto& b : *content.getArray()) {
                    const auto type = b["type"].toString();
                    if (type == "text") text << b["text"].toString();
                    else if (type == "tool_use") {
                        auto fn = obj();
                        set(fn, "name", b["name"]);
                        set(fn, "arguments", b["input"]);
                        auto call = obj();
                        set(call, "function", fn);
                        calls.add(call);
                    }
                }
                auto o = obj();
                set(o, "role", "assistant");
                set(o, "content", text);
                if (!calls.isEmpty()) set(o, "tool_calls", calls);
                out.add(o);
            } else {
                for (const auto& b : *content.getArray()) {
                    if (b["type"].toString() != "tool_result") continue;
                    auto o = obj();
                    set(o, "role", "tool");
                    set(o, "content", b["content"].toString());
                    out.add(o);
                }
            }
        }
    return out;
}

juce::var ollamaToolsFromAnthropic(const juce::var& tools) {
    juce::Array<juce::var> out;
    if (const auto* arr = tools.getArray())
        for (const auto& t : *arr) {
            auto fn = obj();
            set(fn, "name", t["name"]);
            set(fn, "description", t["description"]);
            set(fn, "parameters", t["input_schema"]);
            auto o = obj();
            set(o, "type", "function");
            set(o, "function", fn);
            out.add(o);
        }
    return out;
}

juce::var anthropicResponseFromOllama(const juce::var& ollamaResponse) {
    const auto msg = ollamaResponse["message"];
    juce::Array<juce::var> content;
    const auto text = msg["content"].toString();
    if (text.isNotEmpty()) {
        auto b = obj();
        set(b, "type", "text");
        set(b, "text", text);
        content.add(b);
    }
    bool hasTools = false;
    if (const auto* calls = msg["tool_calls"].getArray()) {
        int i = 0;
        for (const auto& c : *calls) {
            const auto fn = c["function"];
            juce::var input = fn["arguments"];
            if (input.isString()) input = juce::JSON::parse(input.toString());
            auto b = obj();
            set(b, "type", "tool_use");
            set(b, "id", "call_" + juce::String(i++));
            set(b, "name", fn["name"]);
            set(b, "input", input);
            content.add(b);
            hasTools = true;
        }
    }
    auto r = obj();
    set(r, "content", content);
    set(r, "stop_reason", hasTools ? "tool_use" : "end_turn");
    return r;
}

}
