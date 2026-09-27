// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <memory>
#include <string>
#include <vector>

#include "gui/host/BrickHost.h"
#include "gui/host/HostScheduler.h"

namespace hum {


struct GamepadSnapshot {
    static constexpr int kAxes = 6;
    static constexpr int kButtons = 4;
    std::string name;
    float axes[kAxes] = {0.5f, 0.5f, 0.5f, 0.5f, 0.0f, 0.0f};
    float buttons[kButtons] = {};
};

void gamepadPlatformPoll(std::vector<GamepadSnapshot>& out);

class GamepadHost {
public:
    static constexpr int kPollMs = 33;

    GamepadHost(BrickHost& host, HostScheduler& scheduler) : host_(host), scheduler_(scheduler) {}

    void setEnabled(bool on);
    bool enabled() const { return enabled_; }

    int connectedCount() const { return (int) names_.size(); }
    std::string statusText() const;

private:
    void poll();

    struct Last {
        float axes[GamepadSnapshot::kAxes] = {};
        float buttons[GamepadSnapshot::kButtons] = {};
        bool seeded = false;
    };

    BrickHost& host_;
    HostScheduler& scheduler_;
    std::unique_ptr<HostRepeat> polling_;
    std::vector<Last> last_;
    std::vector<std::string> names_;
    bool enabled_ = false;
};

}
