// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/plugins/EmbeddedPluginView.h"

#include <cmath>
#include <cstring>
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace hum {

namespace {

constexpr const wchar_t* kClipWindowClass = L"HumusEmbeddedPluginClip";

LRESULT CALLBACK clipWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_ERASEBKGND) return 1;
    return DefWindowProcW(window, message, wParam, lParam);
}

HINSTANCE codeModule() { return static_cast<HINSTANCE>(juce::Process::getCurrentModuleInstanceHandle()); }

HWND makeClipWindow(HWND parent) {
    static const bool registered = [] {
        WNDCLASSEXW windowClass{};
        windowClass.cbSize = sizeof(windowClass);
        windowClass.lpfnWndProc = clipWindowProc;
        windowClass.hInstance = codeModule();
        windowClass.lpszClassName = kClipWindowClass;
        return RegisterClassExW(&windowClass) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
    }();
    if (!registered) return nullptr;
    return CreateWindowExW(0, kClipWindowClass, L"", WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 0, 0,
                           parent, nullptr, codeModule(), nullptr);
}

struct EmbeddedClipState {
    float scale = 1.0f;
    HWND frame = nullptr;
    HWND canvas = nullptr;
    std::vector<HWND> views;
    juce::Rectangle<int> frameArea, canvasArea, viewsArea;
};

void findNativeViews(juce::Component& c, std::vector<juce::HWNDComponent*>& out) {
    if (auto* native = dynamic_cast<juce::HWNDComponent*>(&c)) {
        out.push_back(native);
        return;
    }
    for (auto* child : c.getChildren()) findNativeViews(*child, out);
}

juce::Rectangle<int> areaInWindow(HWND window, HWND reference) {
    RECT r{};
    GetWindowRect(window, &r);
    MapWindowPoints(HWND_DESKTOP, reference, reinterpret_cast<POINT*>(&r), 2);
    return juce::Rectangle<int>::leftTopRightBottom(r.left, r.top, r.right, r.bottom);
}

juce::Rectangle<int> viewsAreaIn(const std::vector<HWND>& views, HWND peerWindow) {
    juce::Rectangle<int> area;
    for (auto view : views) area = area.getUnion(areaInWindow(view, peerWindow));
    return area;
}

juce::Rectangle<int> shownThroughAncestors(const juce::Component& editor, juce::Component& peerComponent) {
    auto shown = peerComponent.getLocalBounds();
    for (auto* c = editor.getParentComponent(); c != nullptr && c != &peerComponent; c = c->getParentComponent())
        shown = shown.getIntersection(peerComponent.getLocalArea(c, c->getLocalBounds()));
    return shown;
}

bool ensureClipWindows(EmbeddedClipState& state, HWND peerWindow) {
    if (!IsWindow(state.frame)) {
        state.frame = makeClipWindow(peerWindow);
        state.canvas = state.frame != nullptr ? makeClipWindow(state.frame) : nullptr;
        state.frameArea = state.canvasArea = {};
        if (state.canvas == nullptr) {
            if (state.frame != nullptr) DestroyWindow(state.frame);
            state.frame = nullptr;
            return false;
        }
        ShowWindow(state.canvas, SW_SHOWNA);
    } else if (GetParent(state.frame) != peerWindow) {
        SetParent(state.frame, peerWindow);
        state.frameArea = {};
    }
    return true;
}

HRGN regionOf(juce::Rectangle<int> r) { return CreateRectRgn(r.getX(), r.getY(), r.getRight(), r.getBottom()); }

void invalidateDifference(HWND window, juce::Rectangle<int> shown, juce::Rectangle<int> hidden, UINT flags) {
    HRGN area = regionOf(shown);
    HRGN keep = regionOf(hidden);
    if (CombineRgn(area, area, keep, RGN_DIFF) != NULLREGION)
        RedrawWindow(window, nullptr, area, RDW_INVALIDATE | flags);
    DeleteObject(keep);
    DeleteObject(area);
}

void placeClipWindows(EmbeddedClipState& state, juce::Rectangle<int> frameArea, HWND peerWindow) {
    constexpr UINT kQuietMove = SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER;
    constexpr UINT kInPlace = kQuietMove | SWP_NOREDRAW | SWP_NOCOPYBITS;
    if (frameArea.isEmpty()) {
        ShowWindow(state.frame, SW_HIDE);
        state.frameArea = {};
        return;
    }
    const auto before = state.frameArea;
    const UINT move = before.isEmpty() ? kQuietMove : kInPlace;
    const juce::Rectangle<int> canvasArea(-frameArea.getX(), -frameArea.getY(), frameArea.getRight(),
                                          frameArea.getBottom());
    if (canvasArea != state.canvasArea) {
        SetWindowPos(state.canvas, nullptr, canvasArea.getX(), canvasArea.getY(), canvasArea.getWidth(),
                     canvasArea.getHeight(), move);
        state.canvasArea = canvasArea;
    }
    if (frameArea != before) {
        SetWindowPos(state.frame, nullptr, frameArea.getX(), frameArea.getY(), frameArea.getWidth(),
                     frameArea.getHeight(), move | SWP_SHOWWINDOW);
        state.frameArea = frameArea;
        if (!before.isEmpty()) {
            const auto origin = frameArea.getPosition();
            invalidateDifference(state.frame, frameArea - origin, before - origin, RDW_ALLCHILDREN);
            RedrawWindow(state.frame, nullptr, nullptr, RDW_UPDATENOW | RDW_ALLCHILDREN);
            invalidateDifference(peerWindow, before, frameArea, RDW_NOCHILDREN | RDW_UPDATENOW);
        }
    }
}

void returnViewsTo(HWND canvas, HWND parent) {
    for (HWND child = GetWindow(canvas, GW_CHILD); child != nullptr; child = GetWindow(canvas, GW_CHILD))
        if (SetParent(child, parent) == nullptr) break;
}

}

bool embeddedClipUpdate(juce::Component& editor, juce::ComponentPeer& peer, juce::Rectangle<int> clipRegionInPeer,
                        juce::Point<int>, float scale, juce::Colour, void*& handle, juce::Point<int> panInPlugin) {
    std::vector<juce::HWNDComponent*> found;
    findNativeViews(editor, found);
    if (found.empty()) return false;
    auto* state = static_cast<EmbeddedClipState*>(handle);
    if (state == nullptr) {
        state = new EmbeddedClipState;
        handle = state;
    }
    if (auto* processorEditor = dynamic_cast<juce::AudioProcessorEditor*>(&editor);
        processorEditor != nullptr && std::abs(state->scale - scale) > 0.001f) {
        state->scale = scale;
        processorEditor->setScaleFactor(scale);
    }
    editor.setTopLeftPosition(-panInPlugin.x, -panInPlugin.y);
    const auto peerWindow = static_cast<HWND>(peer.getNativeHandle());
    if (!ensureClipWindows(*state, peerWindow)) return true;
    state->views.clear();
    for (auto* view : found)
        if (auto window = static_cast<HWND>(view->getHWND())) state->views.push_back(window);
    const auto shown = clipRegionInPeer.getIntersection(shownThroughAncestors(editor, peer.getComponent()));
    const auto clip = (shown.toFloat() * (float) peer.getPlatformScaleFactor()).toNearestIntEdges();
    state->viewsArea = viewsAreaIn(state->views, peerWindow);
    placeClipWindows(*state, clip.getIntersection(state->viewsArea), peerWindow);
    for (auto view : state->views)
        if (GetParent(view) != state->canvas) SetParent(view, state->canvas);
    return true;
}

bool embeddedClipIntact(void* handle) {
    const auto* state = static_cast<const EmbeddedClipState*>(handle);
    if (state == nullptr) return true;
    if (!IsWindow(state->frame)) return false;
    for (auto view : state->views)
        if (!IsWindow(view) || GetParent(view) != state->canvas) return false;
    return viewsAreaIn(state->views, GetParent(state->frame)) == state->viewsArea;
}

bool embeddedClipSnapshot(void* handle, juce::Image& picture, juce::Rectangle<int>& areaInPeer) {
    const auto* state = static_cast<const EmbeddedClipState*>(handle);
    if (state == nullptr || !IsWindow(state->frame) || !IsWindowVisible(state->frame) || state->frameArea.isEmpty())
        return false;
    RECT screen{};
    GetWindowRect(state->frame, &screen);
    const int w = screen.right - screen.left, h = screen.bottom - screen.top;
    if (w <= 0 || h <= 0) return false;
    HDC screenDc = GetDC(nullptr);
    HDC copyDc = CreateCompatibleDC(screenDc);
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = w;
    info.bmiHeader.biHeight = -h;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr;
    HBITMAP dib = CreateDIBSection(copyDc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    bool copied = false;
    if (dib != nullptr && bits != nullptr) {
        auto previous = SelectObject(copyDc, dib);
        copied = BitBlt(copyDc, 0, 0, w, h, screenDc, screen.left, screen.top, SRCCOPY | CAPTUREBLT) != 0;
        SelectObject(copyDc, previous);
        if (copied) {
            juce::Image image(juce::Image::ARGB, w, h, false, juce::SoftwareImageType());
            juce::Image::BitmapData out(image, juce::Image::BitmapData::writeOnly);
            const auto* rows = static_cast<const juce::uint8*>(bits);
            for (int y = 0; y < h; ++y) {
                auto* line = out.getLinePointer(y);
                std::memcpy(line, rows + (size_t) y * (size_t) w * 4u, (size_t) w * 4u);
                for (int x = 0; x < w; ++x) line[x * 4 + 3] = 0xff;
            }
            picture = image;
            areaInPeer = state->frameArea;
        }
    }
    if (dib != nullptr) DeleteObject(dib);
    DeleteDC(copyDc);
    ReleaseDC(nullptr, screenDc);
    return copied;
}

bool embeddedClipHide(void*& handle) {
    if (auto* state = static_cast<EmbeddedClipState*>(handle); state != nullptr && IsWindow(state->frame)) {
        ShowWindow(state->frame, SW_HIDE);
        state->frameArea = {};
    }
    return false;
}

float embeddedClipScale(void* handle) {
    const auto* state = static_cast<const EmbeddedClipState*>(handle);
    return state != nullptr ? state->scale : 1.0f;
}

void embeddedClipRemove(juce::Component& editor, juce::ComponentPeer* peer, void*& handle) {
    auto* state = static_cast<EmbeddedClipState*>(handle);
    if (state != nullptr && IsWindow(state->frame)) {
        returnViewsTo(state->canvas, peer != nullptr ? static_cast<HWND>(peer->getNativeHandle()) : nullptr);
        DestroyWindow(state->frame);
    }
    std::vector<juce::HWNDComponent*> found;
    findNativeViews(editor, found);
    for (auto* view : found) view->updateHWNDBounds();
    if (auto* processorEditor = dynamic_cast<juce::AudioProcessorEditor*>(&editor))
        processorEditor->setScaleFactor(1.0f);
    delete state;
    handle = nullptr;
}

}
