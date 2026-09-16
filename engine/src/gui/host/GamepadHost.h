// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include <juce_events/juce_events.h>

#include "gui/host/BrickHost.h"

namespace hum {


struct GamepadSnapshot {
    static constexpr int kAxes = 6;
    static constexpr int kButtons = 4;
    std::string name;
    float axes[kAxes] = {0.5f, 0.5f, 0.5f, 0.5f, 0.0f, 0.0f};
    float buttons[kButtons] = {};
};

void gamepadPlatformPoll(std::vector<GamepadSnapshot>& out);

class GamepadHost : private juce::Timer {
public:
    explicit GamepadHost(BrickHost& host) : host_(host) {}
    ~GamepadHost() override { stopTimer(); }

    void setEnabled(bool on);
    bool enabled() const { return enabled_; }

    int connectedCount() const { return (int) names_.size(); }
    std::string statusText() const;

private:
    void timerCallback() override;

    struct Last {
        float axes[GamepadSnapshot::kAxes] = {};
        float buttons[GamepadSnapshot::kButtons] = {};
        bool seeded = false;
    };

    BrickHost& host_;
    std::vector<Last> last_;
    std::vector<std::string> names_;
    bool enabled_ = false;
};

}
