// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>

#include "gui/editor/views/BrickView.h"

namespace hum {

class ToggleView : public virtual BrickView {
public:
    virtual void showOn(bool on) = 0;
    virtual bool shownOn() const = 0;

    std::function<void(bool on)> onToggle;
};

class MomentaryView : public virtual BrickView {
public:
    virtual void showHeld(bool held) = 0;
    virtual bool shownHeld() const = 0;
    virtual void showLamp(bool on, int tint, bool dim) = 0;

    std::function<void(double value)> onWrite;
    std::function<int()> coverTicks;
};

}
