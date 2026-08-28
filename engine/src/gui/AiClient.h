#pragma once
#include <functional>
#include <memory>

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

#include "core/OllamaWire.h"
#include "gui/AppSettings.h"

namespace hum {

inline constexpr const char* kDefaultAiEndpoint = "http://localhost:11434";

class AiClient {
public:
    static juce::String provider() {
        auto p = AppSettings::instance().getString("ai.provider", "ollama").trim();
        return p == "claude" ? p : juce::String("ollama");
    }
    static juce::String endpoint() {
        auto e = AppSettings::instance()
                     .getString("ai.endpoint", kDefaultAiEndpoint).trim();
        return e.endsWithChar('/') ? e.dropLastCharacters(1) : e;
    }
    static juce::String model() {
        const auto m = AppSettings::instance().getString("ai.model").trim();
        if (m.isNotEmpty()) return m;
        return provider() == "claude" ? "claude-sonnet-5" : "llama3.1";
    }

    static juce::String apiKey() {
        auto key = AppSettings::instance().getString("ai.apiKey");
        if (key.isEmpty())
            key = juce::SystemStats::getEnvironmentVariable("ANTHROPIC_API_KEY", {});
        return key.trim();
    }
    static void setApiKey(const juce::String& key) {
        AppSettings::instance().set("ai.apiKey", key.trim());
    }
    static bool needsApiKey() { return provider() == "claude" && apiKey().isEmpty(); }

    struct Request {
        juce::String system;
        juce::String user;
        juce::String model;
        int maxTokens = 1024;
    };
    static void complete(Request req,
                         std::function<void(juce::String text, juce::String error)> onDone) {
        ChatRequest chatReq;
        chatReq.system = std::move(req.system);
        chatReq.model = std::move(req.model);
        chatReq.maxTokens = req.maxTokens;
        auto* msg = new juce::DynamicObject();
        msg->setProperty("role", "user");
        msg->setProperty("content", req.user);
        juce::Array<juce::var> messages;
        messages.add(juce::var(msg));
        chatReq.messages = messages;
        chat(std::move(chatReq), [onDone = std::move(onDone)](juce::var response,
                                                              juce::String error) {
            juce::String text;
            if (const auto* blocks = response["content"].getArray())
                for (const auto& b : *blocks)
                    if (b["type"].toString() == "text") text << b["text"].toString();
            if (error.isEmpty() && text.isEmpty()) error = "Empty reply from the model";
            if (onDone) onDone(text, error);
        });
    }

    struct ChatRequest {
        juce::String system;
        juce::var messages;
        juce::var tools;
        juce::String model;
        int maxTokens = 2048;
    };
    static void chat(ChatRequest req,
                     std::function<void(juce::var response, juce::String error)> onDone) {
        if (req.model.isEmpty()) req.model = model();
        const bool useClaude = provider() == "claude";
        if (useClaude && apiKey().isEmpty()) {
            if (onDone) onDone({}, "No Claude API key set (Settings > AI)");
            return;
        }
        juce::Thread::launch([req = std::move(req), useClaude,
                              key = apiKey(), base = endpoint(),
                              onDone = std::move(onDone)] {
            juce::var response;
            juce::String error;
            if (useClaude) fetchClaude(req, key, response, error);
            else           fetchOllama(req, base, response, error);
            juce::MessageManager::callAsync([onDone, response, error] {
                if (onDone) onDone(response, error);
            });
        });
    }

private:
    static void fetchClaude(const ChatRequest& req, const juce::String& key,
                            juce::var& response, juce::String& error) {
        auto* body = new juce::DynamicObject();
        body->setProperty("model", req.model);
        body->setProperty("max_tokens", req.maxTokens);
        if (req.system.isNotEmpty()) body->setProperty("system", req.system);
        body->setProperty("messages", req.messages);
        if (!req.tools.isVoid()) body->setProperty("tools", req.tools);

        const auto raw = post("https://api.anthropic.com/v1/messages",
                              juce::JSON::toString(juce::var(body), true),
                              "x-api-key: " + key + "\r\nanthropic-version: 2023-06-01",
                              30000, error);
        if (error.isNotEmpty()) return;
        response = juce::JSON::parse(raw);
        if (auto errVar = response["error"]; errVar.isObject())
            error = "API error: " + errVar["message"].toString();
        else if (!response["content"].isArray())
            error = "Malformed reply from the API";
    }

    static void fetchOllama(const ChatRequest& req, const juce::String& base,
                            juce::var& response, juce::String& error) {
        auto* body = new juce::DynamicObject();
        body->setProperty("model", req.model);
        body->setProperty("stream", false);
        body->setProperty("messages", ollamaMessagesFromAnthropic(req.system, req.messages));
        if (!req.tools.isVoid()) body->setProperty("tools", ollamaToolsFromAnthropic(req.tools));

        const auto raw = post(base + "/api/chat",
                              juce::JSON::toString(juce::var(body), true), {}, 180000, error);
        if (error.isNotEmpty()) { error << " (Ollama at " << base << ")"; return; }
        const auto r = juce::JSON::parse(raw);
        if (r["error"].isString()) {
            error = "Ollama: " + r["error"].toString();
            return;
        }
        if (!r["message"].isObject()) {
            error = "Malformed reply from Ollama at " + base;
            return;
        }
        response = anthropicResponseFromOllama(r);
    }

    static juce::String post(const juce::String& url, const juce::String& json,
                             const juce::String& extraHeaders, int timeoutMs,
                             juce::String& error) {
        int status = 0;
        auto headers = juce::String("content-type: application/json");
        if (extraHeaders.isNotEmpty()) headers = extraHeaders + "\r\n" + headers;
        auto stream = juce::URL(url).withPOSTData(json).createInputStream(
            juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
                .withExtraHeaders(headers)
                .withConnectionTimeoutMs(timeoutMs)
                .withStatusCode(&status));
        if (!stream) {
            error = "Could not reach " + juce::URL(url).getDomain()
                    + (status > 0 ? " (HTTP " + juce::String(status) + ")" : juce::String());
            return {};
        }
        return stream->readEntireStreamAsString();
    }
};

}
