// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <chrono>
#include <functional>
#include <string>

namespace hum {

enum class DeviceAnswer { Answered, Silent, Unknown };

using DeviceProbe = std::function<DeviceAnswer(const std::string& device)>;

inline constexpr auto kDeviceFirstAudioWait = std::chrono::seconds(3);

DeviceAnswer probeDeviceStart(const std::string& device);

}
