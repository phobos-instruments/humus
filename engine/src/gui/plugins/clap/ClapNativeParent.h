// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include <clap/ext/gui.h>

namespace hum::claphost {

class NativeParent {
public:
    static std::unique_ptr<NativeParent> make();
    virtual ~NativeParent() = default;

    virtual juce::Component& component() = 0;
    virtual clap_window_t window() = 0;
    virtual void settle() {}
};

}
