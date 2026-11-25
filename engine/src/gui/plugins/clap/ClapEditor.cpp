// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/plugins/clap/ClapEditor.h"

#include <cmath>

#include "gui/plugins/clap/ClapInstance.h"

namespace hum::claphost {

namespace {
float displayScale() {
#if JUCE_MAC
    return 1.0f;
#else
    const auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
    return display != nullptr ? (float) juce::jmax(1.0, display->scale) : 1.0f;
#endif
}
}

Editor::Editor(Instance& instance)
    : AudioProcessorEditor(instance), instance_(instance), plugin_(instance.session().plugin()),
      gui_(instance.session().ext().gui), scale_(displayScale()) {
    setOpaque(true);
    created_ = gui_ != nullptr && gui_->create(plugin_, windowApi(), false);
    if (!created_) {
        setSize(320, 80);
        return;
    }
    if (scale_ != 1.0f) gui_->set_scale(plugin_, scale_);
    uint32_t w = 400, h = 300;
    gui_->get_size(plugin_, &w, &h);
    const bool resizable = gui_->can_resize(plugin_);
    native_ = NativeParent::make();
    addAndMakeVisible(native_->component());
    const auto size = logical(w, h);
    applying_ = true;
    setResizable(resizable, false);
    setSize(juce::jmax(16, size.x), juce::jmax(16, size.y));
    applying_ = false;
    attach();
    instance_.onResizeRequest = [this](uint32_t pw, uint32_t ph) { return resizeFromPlugin(pw, ph); };
}

Editor::~Editor() {
    instance_.onResizeRequest = nullptr;
    if (created_) {
        if (attached_) gui_->hide(plugin_);
        gui_->destroy(plugin_);
    }
    if (native_ != nullptr) removeChildComponent(&native_->component());
    native_.reset();
}

void Editor::attach() {
    native_->component().setBounds(getLocalBounds());
    const clap_window_t window = native_->window();
    attached_ = (window.ptr != nullptr || window.x11 != 0) && gui_->set_parent(plugin_, &window);
    if (!attached_) return;
    gui_->show(plugin_);
    native_->settle();
}

void Editor::resized() {
    if (native_ == nullptr) return;
    native_->component().setBounds(getLocalBounds());
    native_->settle();
    if (applying_ || !attached_ || !gui_->can_resize(plugin_)) return;
    uint32_t w = physical(getWidth()), h = physical(getHeight());
    gui_->adjust_size(plugin_, &w, &h);
    gui_->set_size(plugin_, w, h);
    const auto fitted = logical(w, h);
    if (fitted.x != getWidth() || fitted.y != getHeight()) {
        applying_ = true;
        setSize(fitted.x, fitted.y);
        applying_ = false;
    }
}

bool Editor::resizeFromPlugin(uint32_t w, uint32_t h) {
    const auto size = logical(w, h);
    applying_ = true;
    setSize(juce::jmax(16, size.x), juce::jmax(16, size.y));
    applying_ = false;
    return true;
}

juce::Point<int> Editor::logical(uint32_t w, uint32_t h) const {
    return {(int) std::lround(w / scale_), (int) std::lround(h / scale_)};
}

uint32_t Editor::physical(int logicalSize) const {
    return (uint32_t) juce::jmax(1L, std::lround(logicalSize * scale_));
}

}
