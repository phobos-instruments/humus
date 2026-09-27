// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <optional>
#include <string>

#include "gui/editor/KnobModel.h"
#include "gui/editor/views/BrickView.h"

namespace hum {

struct ValueLook {
    int family = 0;
    int decimals = 0;
    std::optional<double> resetTo;
};

class ValueView : public virtual BrickView {
public:
    virtual void showValue(double value) = 0;
    virtual void showLiveValue(double value) = 0;
    virtual double shownValue() const = 0;
    virtual std::string shownText() = 0;
    virtual void showMeter(float level) = 0;
    virtual void showMarks(bool externallyControlled, bool rollLocked) = 0;
    virtual int textWidthFor(const std::string& text) const = 0;
    virtual void setTextBox(int width, int height) = 0;

    std::function<void(knob::Intent, double)> onIntent;
    std::function<std::string()> tooltipText;
};

}
