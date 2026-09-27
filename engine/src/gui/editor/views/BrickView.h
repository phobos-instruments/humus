// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <string>

#include "gui/editor/Geometry.h"

namespace hum {

class BrickView {
public:
    virtual ~BrickView() = default;

    virtual void setViewBounds(Rect bounds) = 0;
    virtual void setViewVisible(bool visible) = 0;
    virtual bool viewVisible() const = 0;
    virtual void setViewFade(float alpha, bool enabled) = 0;
    virtual float viewAlpha() const = 0;
    virtual bool viewEnabled() const = 0;
    virtual void setViewTooltip(const std::string& tip) = 0;
    virtual void showMarks(bool externallyControlled, bool rollLocked) {
        (void) externallyControlled;
        (void) rollLocked;
    }

    std::function<void(Point)> onMenu;
};

enum class LabelKind { Plain, Pill, Section, Above, Beside };

class LabelView : public virtual BrickView {
public:
    virtual void showText(const std::string& text) = 0;
    virtual int textWidth() const = 0;
};

}
