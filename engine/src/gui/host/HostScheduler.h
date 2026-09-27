// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <memory>

namespace hum {

class HostRepeat {
public:
    virtual ~HostRepeat() = default;
};

class HostScheduler {
public:
    virtual ~HostScheduler() = default;
    virtual void post(std::function<void()> job) = 0;
    virtual std::unique_ptr<HostRepeat> repeat(int intervalMs, std::function<void()> tick) = 0;
};

HostScheduler& messageThreadScheduler();

}
