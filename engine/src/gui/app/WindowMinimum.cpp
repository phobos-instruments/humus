// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#if defined(__linux__) && !defined(JUCE_GUI_BASICS_INCLUDE_XHEADERS)
 #define JUCE_GUI_BASICS_INCLUDE_XHEADERS 1
#endif

#include "gui/app/WindowMinimum.h"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace hum {

juce::BorderSize<int> windowFrameSize(const juce::ResizableWindow& w) {
    if (auto* peer = w.getPeer())
        if (const auto& frame = peer->getFrameSizeIfPresent()) return *frame;
    return {};
}

#if JUCE_LINUX

namespace {

struct XTarget {
    ::Display* display = nullptr;
    ::Window handle = 0;
    double scale = 1.0;
    bool valid() const { return display != nullptr && handle != 0; }
};

XTarget xTargetFor(const juce::ResizableWindow& w) {
    XTarget t;
    auto* peer = w.getPeer();
    if (peer == nullptr) return t;
    t.display = juce::XWindowSystem::getInstance()->getDisplay();
    t.handle = (::Window) (juce::pointer_sized_int) peer->getNativeHandle();
    t.scale = peer->getPlatformScaleFactor();
    return t;
}

constexpr long kSizeHintsFields = 18;

void writeMinimumHints(const juce::ResizableWindow& w, int contentW, int contentH) {
    const auto t = xTargetFor(w);
    if (!t.valid()) return;
    const auto* peer = w.getPeer();
    const auto where = peer->getBounds();
    juce::XWindowSystemUtilities::ScopedXLock lock;
    auto* symbols = juce::X11Symbols::getInstance();
    if (auto* hints = symbols->xAllocSizeHints()) {
        hints->flags = PMinSize | USPosition | USSize;
        hints->x = juce::roundToInt(t.scale * where.getX());
        hints->y = juce::roundToInt(t.scale * where.getY());
        hints->width = juce::jmax(1, juce::roundToInt(t.scale * where.getWidth()));
        hints->height = juce::jmax(1, juce::roundToInt(t.scale * where.getHeight()));
        hints->min_width = juce::jmax(1, juce::roundToInt(t.scale * contentW));
        hints->min_height = juce::jmax(1, juce::roundToInt(t.scale * contentH));
        symbols->xSetWMNormalHints(t.display, t.handle, hints);
        symbols->xFree(hints);
    }
}

}

namespace {

constexpr int kKeepMs = 200;

class MinimumKeeper : private juce::Timer {
public:
    static MinimumKeeper& instance() {
        static MinimumKeeper keeper;
        return keeper;
    }

    void keep(const juce::ResizableWindow& w, int contentW, int contentH) {
        auto* key = const_cast<juce::ResizableWindow*>(&w);
        for (auto& e : kept_)
            if (e.window.getComponent() == key) {
                e.w = contentW;
                e.h = contentH;
                return;
            }
        kept_.push_back({juce::Component::SafePointer<juce::Component>(key), contentW, contentH});
        if (!isTimerRunning()) startTimer(kKeepMs);
    }

private:
    struct Entry {
        juce::Component::SafePointer<juce::Component> window;
        int w = 0, h = 0;
    };

    void timerCallback() override {
        kept_.erase(std::remove_if(kept_.begin(), kept_.end(),
                                   [](const Entry& e) { return e.window.getComponent() == nullptr; }),
                    kept_.end());
        for (auto& e : kept_)
            if (auto* again = dynamic_cast<juce::ResizableWindow*>(e.window.getComponent()))
                writeMinimumHints(*again, e.w, e.h);
        if (kept_.empty()) stopTimer();
    }

    std::vector<Entry> kept_;
};

}

void refreshWindowMinimum(const juce::ResizableWindow& w, int contentW, int contentH) {
    writeMinimumHints(w, contentW, contentH);
    MinimumKeeper::instance().keep(w, contentW, contentH);
}

juce::Point<int> windowManagerMinimum(const juce::ResizableWindow& w) {
    const auto t = xTargetFor(w);
    if (!t.valid()) return {};
    juce::XWindowSystemUtilities::ScopedXLock lock;
    Atom type = None;
    int format = 0;
    unsigned long count = 0, bytesLeft = 0;
    unsigned char* raw = nullptr;
    juce::Point<int> got;
    if (juce::X11Symbols::getInstance()->xGetWindowProperty(
            t.display, t.handle, XA_WM_NORMAL_HINTS, 0, kSizeHintsFields, False,
            XA_WM_SIZE_HINTS, &type, &format, &count, &bytesLeft, &raw) == Success
        && raw != nullptr) {
        const auto* field = reinterpret_cast<const long*>(raw);
        if (count >= 7 && (field[0] & PMinSize) != 0)
            got = {juce::roundToInt((double) field[5] / t.scale),
                   juce::roundToInt((double) field[6] / t.scale)};
        juce::X11Symbols::getInstance()->xFree(raw);
    }
    return got;
}

#else

void refreshWindowMinimum(const juce::ResizableWindow&, int, int) {}

juce::Point<int> windowManagerMinimum(const juce::ResizableWindow& w) {
    const auto* c = const_cast<juce::ResizableWindow&>(w).getConstrainer();
    if (c == nullptr) return {};
    const auto frame = windowFrameSize(w);
    return {c->getMinimumWidth() - frame.getLeftAndRight(),
            c->getMinimumHeight() - frame.getTopAndBottom()};
}

#endif

void setWindowMinimumSize(juce::ResizableWindow& w, int contentW, int contentH) {
    const auto frame = windowFrameSize(w);
    const int minW = contentW + frame.getLeftAndRight();
    const int minH = contentH + frame.getTopAndBottom();
    const auto& displays = juce::Desktop::getInstance().getDisplays();
    const auto* on = displays.getDisplayForRect(w.getBounds());
    if (on == nullptr) on = displays.getPrimaryDisplay();
    const auto room = on != nullptr ? on->userArea
                                    : juce::Rectangle<int>(0, 0, kWindowMaxSide, kWindowMaxSide);
    const int wantW = juce::jlimit(kWindowFloorW, juce::jmax(kWindowFloorW, room.getWidth()), minW);
    const int wantH = juce::jlimit(kWindowFloorH, juce::jmax(kWindowFloorH, room.getHeight()), minH);
    w.setResizeLimits(wantW, wantH, kWindowMaxSide, kWindowMaxSide);
    refreshWindowMinimum(w, wantW - frame.getLeftAndRight(), wantH - frame.getTopAndBottom());
}

}
