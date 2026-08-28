#pragma once
#if defined(HUM_X11_MIRROR)
#include <cstring>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "gui/X11Mirror.h"

namespace hum {
namespace {
int swallowXErrors(Display*, XErrorEvent*) { return 0; }

struct ErrorTrap {
    explicit ErrorTrap(Display* d) : dpy(d), old(XSetErrorHandler(swallowXErrors)) {}
    ~ErrorTrap() { XSync(dpy, False); XSetErrorHandler(old); }
    Display* dpy;
    XErrorHandler old;
};
}

struct X11Mirror::Impl {
    Display* dpy = nullptr;
    ::Window win = 0;
    bool redirected = false;

    XImage* ximg = nullptr;
    XShmSegmentInfo shm{};
    bool shmAttached = false;
    bool useShm = false;
    int imgW = 0, imgH = 0;

    juce::Image image;
    int consecutiveFailures = 0;
    bool broken = false;

    ~Impl() {
        if (dpy != nullptr) {
            ErrorTrap trap(dpy);
            freeImage();
            if (redirected) {
                XCompositeUnredirectSubwindows(dpy, win, CompositeRedirectAutomatic);
                XCompositeUnredirectWindow(dpy, win, CompositeRedirectManual);
            }
        }
        if (dpy != nullptr) XCloseDisplay(dpy);
    }

    void freeImage() {
        if (ximg != nullptr) {
            if (shmAttached) XShmDetach(dpy, &shm);
            XDestroyImage(ximg);
            ximg = nullptr;
        }
        if (shm.shmaddr != nullptr && shm.shmaddr != (char*) -1) {
            shmdt(shm.shmaddr);
            shm.shmaddr = nullptr;
        }
        shmAttached = false;
        imgW = imgH = 0;
    }

    bool ensureImage(int w, int h, int depth) {
        if (ximg != nullptr && w == imgW && h == imgH) return true;
        freeImage();

        if (useShm) {
            ximg = XShmCreateImage(dpy, DefaultVisual(dpy, DefaultScreen(dpy)),
                                   (unsigned) depth, ZPixmap, nullptr, &shm,
                                   (unsigned) w, (unsigned) h);
            if (ximg != nullptr) {
                shm.shmid = shmget(IPC_PRIVATE,
                                   (size_t) ximg->bytes_per_line * (size_t) h,
                                   IPC_CREAT | 0600);
                if (shm.shmid >= 0) {
                    shm.shmaddr = ximg->data = (char*) shmat(shm.shmid, nullptr, 0);
                    shm.readOnly = False;
                    if (shm.shmaddr != (char*) -1 && XShmAttach(dpy, &shm)) {
                        shmAttached = true;
                        XSync(dpy, False);
                        shmctl(shm.shmid, IPC_RMID, nullptr);
                    }
                }
                if (!shmAttached) {
                    XDestroyImage(ximg);
                    ximg = nullptr;
                    useShm = false;
                }
            } else {
                useShm = false;
            }
        }
        imgW = w;
        imgH = h;
        image = juce::Image(juce::Image::ARGB, w, h, false);
        return true;
    }

    void convert(XImage* src) {
        juce::Image::BitmapData bd(image, juce::Image::BitmapData::writeOnly);
        const int rowBytes = juce::jmin((int) src->bytes_per_line, bd.lineStride);
        for (int y = 0; y < imgH; ++y) {
            auto* dst = bd.getLinePointer(y);
            std::memcpy(dst, src->data + (size_t) y * (size_t) src->bytes_per_line,
                        (size_t) rowBytes);
            for (int x = 0; x < imgW; ++x) dst[x * 4 + 3] = 0xff;
        }
    }
};

inline bool hasComposite(Display* d) {
    int ev = 0, err = 0, major = 0, minor = 0;
    return XCompositeQueryExtension(d, &ev, &err)
        && XCompositeQueryVersion(d, &major, &minor) && (major > 0 || minor >= 2);
}

}
#endif
