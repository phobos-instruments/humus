// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include "gui/editor/views/BrickView.h"

namespace hum {

class RichOwner {
public:
    virtual ~RichOwner() = default;
    virtual std::string organism() const = 0;
    virtual std::string className() const = 0;
    virtual void automationChanged() = 0;
    virtual void reloadFace() = 0;
    virtual void repaintFace() = 0;
};

class RichView : public virtual BrickView {
public:
    virtual void reloadValues() = 0;
    virtual void reloadText() = 0;
    virtual void refreshLive() = 0;
    virtual void openClip(int clip) = 0;
    virtual std::string shownText() const = 0;
};

}
