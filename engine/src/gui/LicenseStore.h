#pragma once
#include <juce_core/juce_core.h>

#include "core/AppPaths.h"
#include "core/HubPublicKey.h"
#include "core/LicenseCheck.h"
#include "gui/HubClient.h"

namespace hum {

class LicenseStore {
public:
    static juce::File file() { return appDataDir().getChildFile("license.json"); }

    static licensecheck::License current() {
        const auto v = juce::JSON::parse(file().loadFileAsString());
        if (!v.isObject()) return {};
        return licensecheck::verifyLicense(
            v.getProperty("payload_b64", "").toString().toStdString(),
            v.getProperty("sig_b64", "").toString().toStdString(),
            HubClient::instanceId(), kHubPublicKeys, kNumHubPublicKeys);
    }

    static licensecheck::License save(const std::string& payloadB64,
                                      const std::string& sigB64) {
        const auto lic = licensecheck::verifyLicense(
            payloadB64, sigB64, HubClient::instanceId(),
            kHubPublicKeys, kNumHubPublicKeys);
        if (!lic.valid) return lic;
        auto* o = new juce::DynamicObject();
        o->setProperty("payload_b64", juce::String(payloadB64));
        o->setProperty("sig_b64", juce::String(sigB64));
        file().replaceWithText(juce::JSON::toString(juce::var(o)));
        return lic;
    }

    static void remove() { file().deleteFile(); }
};

}
