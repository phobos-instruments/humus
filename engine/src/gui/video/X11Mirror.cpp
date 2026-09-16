// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/video/X11Mirror.h"

#if defined(HUM_X11_MIRROR)
#include "gui/video/X11MirrorImpl.h"

namespace hum {

bool X11Mirror::available() {
    Display* d = XOpenDisplay(nullptr);
    if (d == nullptr) return false;
    const bool comp = hasComposite(d);
    XCloseDisplay(d);
    return comp;
}

X11Mirror::X11Mirror(void* nativeWindowHandle) : impl_(std::make_unique<Impl>()) {
    impl_->dpy = XOpenDisplay(nullptr);
    impl_->win = (::Window) reinterpret_cast<uintptr_t>(nativeWindowHandle);
    if (impl_->dpy == nullptr || impl_->win == 0 || !hasComposite(impl_->dpy)) {
        impl_->broken = true;
        return;
    }
    impl_->useShm = XShmQueryExtension(impl_->dpy) == True;

    ErrorTrap trap(impl_->dpy);
    XCompositeRedirectWindow(impl_->dpy, impl_->win, CompositeRedirectManual);
    XCompositeRedirectSubwindows(impl_->dpy, impl_->win, CompositeRedirectAutomatic);
    impl_->redirected = true;
}

X11Mirror::~X11Mirror() = default;

bool X11Mirror::valid() const {
    return impl_ != nullptr && !impl_->broken && impl_->consecutiveFailures < 30;
}

const juce::Image& X11Mirror::image() const { return impl_->image; }

bool X11Mirror::grab() {
    auto& im = *impl_;
    if (im.broken || im.dpy == nullptr) return false;

    ErrorTrap trap(im.dpy);
    auto fail = [&] { ++im.consecutiveFailures; return false; };

    XWindowAttributes wa{};
    if (XGetWindowAttributes(im.dpy, im.win, &wa) == 0
        || wa.width <= 0 || wa.height <= 0 || wa.map_state != IsViewable)
        return fail();

    im.ensureImage(wa.width, wa.height, wa.depth);

    Pixmap pix = XCompositeNameWindowPixmap(im.dpy, im.win);
    if (pix == 0) return fail();

    bool ok = false;
    if (im.useShm && im.ximg != nullptr) {
        ok = XShmGetImage(im.dpy, pix, im.ximg, 0, 0, AllPlanes) == True;
        if (ok) im.convert(im.ximg);
    } else {
        if (XImage* got = XGetImage(im.dpy, pix, 0, 0, (unsigned) wa.width,
                                    (unsigned) wa.height, AllPlanes, ZPixmap)) {
            im.convert(got);
            XDestroyImage(got);
            ok = true;
        }
    }
    XFreePixmap(im.dpy, pix);

    if (!ok) return fail();
    im.consecutiveFailures = 0;
    return true;
}

int X11Mirror::embeddedClientCount() const {
    if (impl_->broken || impl_->dpy == nullptr) return 0;
    ErrorTrap trap(impl_->dpy);
    auto childrenOf = [this](::Window w, ::Window** out, unsigned& n) {
        ::Window root = 0, parent = 0;
        return XQueryTree(impl_->dpy, w, &root, &parent, out, &n) != 0;
    };
    ::Window* kids = nullptr;
    unsigned nKids = 0;
    if (!childrenOf(impl_->win, &kids, nKids)) return 0;
    int clients = 0;
    for (unsigned i = 0; i < nKids; ++i) {
        ::Window* sub = nullptr;
        unsigned nSub = 0;
        if (childrenOf(kids[i], &sub, nSub)) clients += (int) nSub;
        if (sub != nullptr) XFree(sub);
    }
    if (kids != nullptr) XFree(kids);
    return clients;
}

}

#else
namespace hum {
struct X11Mirror::Impl { juce::Image image; };
bool X11Mirror::available() { return false; }
X11Mirror::X11Mirror(void*) : impl_(std::make_unique<Impl>()) {}
X11Mirror::~X11Mirror() = default;
bool X11Mirror::valid() const { return false; }
bool X11Mirror::grab() { return false; }
int X11Mirror::embeddedClientCount() const { return 0; }
const juce::Image& X11Mirror::image() const { return impl_->image; }
void X11Mirror::sendMouseMove(juce::Point<int>, bool) {}
void X11Mirror::sendMouseButton(juce::Point<int>, int, bool) {}
void X11Mirror::sendWheel(juce::Point<int>, bool) {}
}
#endif
