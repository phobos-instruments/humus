// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <utility>
#include <vector>

#include <juce_core/juce_core.h>

#include "core/packs/PackUpdates.h"

namespace hum {
namespace hubproto {

enum class LicenseStatus { None, Valid, Revoked, Unknown };

struct CheckinResult {
    bool hasUpdate = false;
    std::string updateVersion, updateUrl, updateNotes, updateSha256;
    LicenseStatus licenseStatus = LicenseStatus::None;
};

inline juce::String buildCheckinBody(const std::string& instanceId,
                                     const std::string& appVersion,
                                     const std::string& platform,
                                     const std::string& licenseId,
                                     const std::string& buildId = "") {
    auto* o = new juce::DynamicObject();
    o->setProperty("instance_id", juce::String(instanceId));
    o->setProperty("app_version", juce::String(appVersion));
    o->setProperty("platform", juce::String(platform));
    if (!licenseId.empty())
        o->setProperty("license_id", juce::String(licenseId));
    if (!buildId.empty())
        o->setProperty("build_id", juce::String(buildId));
    return juce::JSON::toString(juce::var(o), true);
}

inline CheckinResult parseCheckin(const juce::String& json,
                                  const std::string& appVersion) {
    CheckinResult r;
    const auto v = juce::JSON::parse(json);
    if (!v.isObject()) return r;

    const auto update = v.getProperty("update", juce::var());
    if (update.isObject()) {
        const std::string ver =
            update.getProperty("version", "").toString().toStdString();
        if (!ver.empty() && compareVersions(ver, appVersion) > 0) {
            r.hasUpdate = true;
            r.updateVersion = ver;
            r.updateUrl = update.getProperty("url", "").toString().toStdString();
            r.updateNotes = update.getProperty("notes", "").toString().toStdString();
            r.updateSha256 =
                update.getProperty("sha256", "").toString().toLowerCase().toStdString();
        }
    }

    const auto s = v.getProperty("license_status", "").toString();
    r.licenseStatus = s == "valid"     ? LicenseStatus::Valid
                      : s == "revoked" ? LicenseStatus::Revoked
                      : s == "none"    ? LicenseStatus::None
                                       : LicenseStatus::Unknown;
    return r;
}

inline juce::String buildTelemetryBody(
    const std::string& instanceId, const std::string& appVersion,
    const std::string& platform, bool crashedLastRun,
    const std::vector<std::pair<std::string, juce::int64>>& events) {
    auto* o = new juce::DynamicObject();
    o->setProperty("instance_id", juce::String(instanceId));
    o->setProperty("app_version", juce::String(appVersion));
    o->setProperty("platform", juce::String(platform));
    o->setProperty("crashed_last_run", crashedLastRun);
    juce::Array<juce::var> arr;
    for (const auto& [name, count] : events) {
        auto* e = new juce::DynamicObject();
        e->setProperty("name", juce::String(name));
        e->setProperty("count", count);
        arr.add(juce::var(e));
    }
    o->setProperty("events", arr);
    return juce::JSON::toString(juce::var(o), true);
}

struct ActivateResult {
    bool ok = false;
    std::string payloadB64, sigB64;
    std::string error;
};

inline ActivateResult parseActivateResponse(const juce::String& json) {
    ActivateResult r;
    const auto v = juce::JSON::parse(json);
    if (!v.isObject()) { r.error = "unreachable"; return r; }
    const auto lic = v.getProperty("license", juce::var());
    if (lic.isObject()) {
        r.payloadB64 = lic.getProperty("payload_b64", "").toString().toStdString();
        r.sigB64 = lic.getProperty("sig_b64", "").toString().toStdString();
        r.ok = !r.payloadB64.empty() && !r.sigB64.empty();
        return r;
    }
    r.error = v.getProperty("detail", "unreachable").toString().toStdString();
    return r;
}

}
}
