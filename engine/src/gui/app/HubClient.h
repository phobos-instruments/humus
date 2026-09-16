// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <string>

#include <HumBuildId.h>
#include <juce_events/juce_events.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "core/net/HubProtocol.h"
#include "core/net/HubPublicKey.h"
#include "core/packs/PackLoader.h"
#include "gui/app/AppSettings.h"

namespace hum {

class HubClient {
public:
    static juce::String baseUrl() {
        const auto u = AppSettings::instance().getString("hub.url", "");
        return (u.isNotEmpty() ? u : juce::String(kDefaultHubUrl))
            .trimCharactersAtEnd("/");
    }

    static std::string instanceId() {
        auto id = AppSettings::instance().getString("telemetry.instanceId", "");
        if (id.isEmpty()) {
            id = juce::Uuid().toString();
            if (juce::JUCEApplication::getInstance() != nullptr)
                AppSettings::instance().set("telemetry.instanceId", id);
        }
        return id.toStdString();
    }

    static std::string appVersion() {
        auto* app = juce::JUCEApplication::getInstance();
        return app != nullptr ? app->getApplicationVersion().toStdString()
                              : std::string("0.0.0");
    }

    static std::string updatePlatform() { return PackLoader::platformTag(); }

    static void checkin(const std::string& licenseId,
                        std::function<void(bool ok, hubproto::CheckinResult)> onDone) {
        const auto body = hubproto::buildCheckinBody(
            instanceId(), appVersion(), updatePlatform(), licenseId,
            HUM_BUILD_ID);
        postAsync("/v1/checkin", body,
                  [onDone = std::move(onDone), ver = appVersion()](int status,
                                                                   juce::String reply) {
                      if (onDone)
                          onDone(status == 200, hubproto::parseCheckin(reply, ver));
                  });
    }

    static void activate(const std::string& code, const std::string& name,
                         std::function<void(hubproto::ActivateResult)> onDone) {
        auto* o = new juce::DynamicObject();
        o->setProperty("code", juce::String(code));
        o->setProperty("instance_id", juce::String(instanceId()));
        o->setProperty("name", juce::String(name));
        postAsync("/v1/license/activate", juce::JSON::toString(juce::var(o), true),
                  [onDone = std::move(onDone)](int, juce::String reply) {
                      if (onDone) onDone(hubproto::parseActivateResponse(reply));
                  });
    }

    static void sendTelemetry(const juce::String& body,
                              std::function<void(bool ok)> onDone) {
        postAsync("/v1/telemetry", body,
                  [onDone = std::move(onDone)](int status, juce::String) {
                      if (onDone) onDone(status == 204);
                  });
    }

private:
    static void postAsync(const juce::String& path, const juce::String& body,
                          std::function<void(int status, juce::String reply)> onDone) {
        juce::Thread::launch([url = baseUrl() + path, body,
                              onDone = std::move(onDone)] {
            int status = 0;
            auto stream = juce::URL(url).withPOSTData(body).createInputStream(
                juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
                    .withExtraHeaders("content-type: application/json")
                    .withConnectionTimeoutMs(4000)
                    .withStatusCode(&status));
            juce::String reply =
                stream != nullptr ? stream->readEntireStreamAsString() : juce::String();
            juce::MessageManager::callAsync(
                [onDone, status, reply] { if (onDone) onDone(status, reply); });
        });
    }
};

}
