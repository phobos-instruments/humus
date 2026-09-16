// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

namespace hum::telemetry {

inline constexpr const char* kBoot = "boot";
inline constexpr const char* kWizardCompleted = "wizard_completed";
inline constexpr const char* kPatchSaved = "patch_saved";
inline constexpr const char* kSessionMinutes = "session_minutes";
inline constexpr const char* kPluginScanFailed = "plugin_scan_failed";
inline constexpr const char* kUpdateShown = "update_shown";
inline constexpr const char* kUpdateTaken = "update_taken";

inline std::string organismAdd(const std::string& cls) { return "add." + cls; }

inline std::string packToggle(const std::string& id, bool on) {
    return (on ? "pack_on." : "pack_off.") + id;
}

}
