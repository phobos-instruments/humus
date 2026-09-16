// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

namespace hum {

class X11Mirror {
public:
    static bool available();

    explicit X11Mirror(void* nativeWindowHandle);
    explicit X11Mirror(unsigned long xid)
        : X11Mirror(reinterpret_cast<void*>(static_cast<std::uintptr_t>(xid))) {}
    ~X11Mirror();

    bool valid() const;

    bool grab();
    const juce::Image& image() const;

    int embeddedClientCount() const;

    void sendMouseMove(juce::Point<int> p, bool leftDown);
    void sendMouseButton(juce::Point<int> p, int button, bool down);
    void sendWheel(juce::Point<int> p, bool up);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(X11Mirror)
};

}
