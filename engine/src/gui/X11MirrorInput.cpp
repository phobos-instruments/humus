#include "gui/X11Mirror.h"

#if defined(HUM_X11_MIRROR)
#include "gui/X11MirrorImpl.h"

namespace hum {

namespace {
::Window deepestChildAt(Display* dpy, ::Window top, int& x, int& y) {
    ::Window cur = top;
    for (;;) {
        ::Window child = 0;
        int cx = 0, cy = 0;
        if (!XTranslateCoordinates(dpy, cur, cur, x, y, &cx, &cy, &child) || child == 0)
            return cur;
        int nx = 0, ny = 0;
        ::Window dummy = 0;
        XTranslateCoordinates(dpy, cur, child, x, y, &nx, &ny, &dummy);
        x = nx;
        y = ny;
        cur = child;
    }
}

void sendButtonEvent(Display* dpy, ::Window top, juce::Point<int> p,
                     unsigned button, bool press, unsigned stateAfter) {
    int x = p.x, y = p.y;
    const ::Window target = deepestChildAt(dpy, top, x, y);
    XButtonEvent e{};
    e.type = press ? ButtonPress : ButtonRelease;
    e.display = dpy; e.window = target; e.root = DefaultRootWindow(dpy);
    e.subwindow = 0; e.time = CurrentTime;
    e.x = x; e.y = y;
    e.x_root = p.x; e.y_root = p.y;
    e.state = stateAfter; e.button = button; e.same_screen = True;
    XSendEvent(dpy, target, True, press ? ButtonPressMask : ButtonReleaseMask,
               (XEvent*) &e);
    XFlush(dpy);
}
}

void X11Mirror::sendMouseButton(juce::Point<int> p, int button, bool down) {
    if (!valid()) return;
    ErrorTrap trap(impl_->dpy);
    const unsigned btn = button == 3 ? Button3 : button == 2 ? Button2 : Button1;
    const unsigned mask = btn == Button1 ? Button1Mask : btn == Button2 ? Button2Mask : Button3Mask;
    sendButtonEvent(impl_->dpy, impl_->win, p, btn, down, down ? 0 : mask);
}

void X11Mirror::sendWheel(juce::Point<int> p, bool up) {
    if (!valid()) return;
    ErrorTrap trap(impl_->dpy);
    const unsigned btn = up ? Button4 : Button5;
    sendButtonEvent(impl_->dpy, impl_->win, p, btn, true, 0);
    sendButtonEvent(impl_->dpy, impl_->win, p, btn, false, 0);
}

void X11Mirror::sendMouseMove(juce::Point<int> p, bool leftDown) {
    if (!valid()) return;
    ErrorTrap trap(impl_->dpy);
    int x = p.x, y = p.y;
    const ::Window target = deepestChildAt(impl_->dpy, impl_->win, x, y);
    XMotionEvent e{};
    e.type = MotionNotify;
    e.display = impl_->dpy; e.window = target; e.root = DefaultRootWindow(impl_->dpy);
    e.time = CurrentTime;
    e.x = x; e.y = y;
    e.x_root = p.x; e.y_root = p.y;
    e.state = leftDown ? Button1Mask : 0;
    e.is_hint = NotifyNormal; e.same_screen = True;
    XSendEvent(impl_->dpy, target, True,
               leftDown ? (PointerMotionMask | Button1MotionMask) : PointerMotionMask,
               (XEvent*) &e);
    XFlush(impl_->dpy);
}

}
#endif
