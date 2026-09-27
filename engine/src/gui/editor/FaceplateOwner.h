// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include "gui/editor/Geometry.h"
#include "gui/editor/MomentaryPress.h"

namespace hum {

class FaceplateOwner {
public:
    virtual ~FaceplateOwner() = default;
    virtual void automationChanged() {}
    virtual void openMenu(const std::string&, Point) {}
    virtual int coverTicks() const { return momentary::coverTicksFor(0.0, 0); }
    virtual unsigned nowMs() const { return 0u; }
    virtual void repaintFace() {}
    virtual void gatesApplied() {}
    virtual bool knowsClass(const std::string&) const { return false; }
};

}
