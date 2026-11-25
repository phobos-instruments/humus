// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/plugins/clap/ClapNativeParent.h"

#include <juce_gui_extra/juce_gui_extra.h>

namespace hum::claphost {

#if JUCE_WINDOWS

namespace {
class WindowsParent : public NativeParent {
public:
    WindowsParent() {
        inner_.setOpaque(true);
        inner_.addToDesktop(0);
        if (auto* peer = inner_.getPeer()) host_.setHWND(peer->getNativeHandle());
        host_.setOpaque(true);
    }
    ~WindowsParent() override { host_.setHWND(nullptr); }

    juce::Component& component() override { return host_; }
    clap_window_t window() override {
        clap_window_t w{};
        w.api = CLAP_WINDOW_API_WIN32;
        w.win32 = host_.getHWND();
        return w;
    }
    void settle() override { host_.updateHWNDBounds(); }

private:
    struct Inner : juce::Component {
        void paint(juce::Graphics& g) override { g.fillAll(juce::Colours::black); }
    };
    Inner inner_;
    juce::HWNDComponent host_;
};
}

std::unique_ptr<NativeParent> NativeParent::make() { return std::make_unique<WindowsParent>(); }

#elif JUCE_LINUX || JUCE_BSD

namespace {
class X11Parent : public NativeParent {
public:
    juce::Component& component() override { return host_; }
    clap_window_t window() override {
        clap_window_t w{};
        w.api = CLAP_WINDOW_API_X11;
        w.x11 = host_.getHostWindowID();
        return w;
    }

private:
    juce::XEmbedComponent host_{true, false};
};
}

std::unique_ptr<NativeParent> NativeParent::make() { return std::make_unique<X11Parent>(); }

#endif

}
