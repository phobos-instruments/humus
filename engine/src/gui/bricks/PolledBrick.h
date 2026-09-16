// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <string>

#include "gui/host/BrickHost.h"
#include "hum/Organism.h"
#include "gui/editor/OrganismEditor.h"
#include "gui/common/UiTicker.h"

namespace hum {

class PolledBrick : public OrganismEditor {
public:
    PolledBrick(BrickHost& host, std::string name, int everyNthTick = 1)
        : host_(host), name_(std::move(name)), stride_(std::max(1, everyNthTick)) {
        tickerId_ = UiTicker::instance().add([this] {
            if (++tick_ >= stride_) {
                tick_ = 0;
                poll();
            }
        });
    }
    ~PolledBrick() override { UiTicker::instance().remove(tickerId_); }

    void reloadValues() override {}
    void refreshAutomatedValues() override {}

protected:
    virtual void poll() = 0;

    template <class Src> Src* live() const {
        return dynamic_cast<Src*>(host_.liveOrganism(name_));
    }

    BrickHost& host_;
    std::string name_;

private:
    int tickerId_ = 0;
    int tick_ = 0;
    const int stride_;
};

}
