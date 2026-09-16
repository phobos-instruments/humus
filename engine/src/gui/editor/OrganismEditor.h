// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

namespace hum {

class OrganismEditor : public juce::Component {
public:
    ~OrganismEditor() override = default;

    virtual void reloadValues() = 0;
    virtual void refreshAutomatedValues() = 0;
    virtual void reloadTextValues() {}
    virtual int preferredContentWidth() const = 0;
    virtual int preferredContentHeight(int width) const = 0;
    virtual void openClip(int) {}
    virtual void setLayoutDeferred(bool) {}

    std::function<void()> onAutomationChanged;

    void setWearsCollar(bool w) {
        if (wearsCollar_ == w) return;
        wearsCollar_ = w;
        resized();
        repaint();
    }
    bool wearsCollar() const { return wearsCollar_; }

private:
    bool wearsCollar_ = true;
};

}
