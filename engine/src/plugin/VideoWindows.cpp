// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "plugin/VideoWindows.h"

#include <utility>

#include "gui/host/EngineHost.h"
#include "gui/video/VisualWindow.h"
#include "gui/video/WindowFloat.h"
#include "hum/caps/Audio.h"
#include "hum/caps/Video.h"
#include "plugin/DesktopSession.h"

namespace hum {

namespace {

VideoWindows::Make visualWindows(EngineHost& host) {
    return [&host](const std::string& name, int width, int height,
                   VideoWindows::OnClosed onClosed) -> std::unique_ptr<juce::DocumentWindow> {
        if (!sessionCanOpenWindows()) return nullptr;
        return std::unique_ptr<juce::DocumentWindow>(
            std::make_unique<VisualWindow>(host, name, nullptr, std::move(onClosed), width, height));
    };
}

}

VideoWindows::VideoWindows(EngineHost& host, Make make)
    : host_(host), make_(make ? std::move(make) : visualWindows(host)) {}

VideoWindows::~VideoWindows() {
    *alive_ = false;
    windows_.clear();
}

bool VideoWindows::showsPicture(const std::string& name) const {
    auto* organism = host_.liveOrganism(name);
    return dynamic_cast<VideoNode*>(organism) != nullptr && dynamic_cast<VisualSource*>(organism) == nullptr;
}

void VideoWindows::open(const std::string& name, int width, int height) {
    if (!showsPicture(name)) return;
    if (const auto it = windows_.find(name); it != windows_.end()) {
        if (auto* visual = dynamic_cast<VisualWindow*>(it->second.get()); visual != nullptr && width > 0 && height > 0)
            visual->openAt(width, height);
        it->second->toFront(true);
        return;
    }
    if (auto window = make_(name, width, height, [this](const std::string& n) { closed(n); }))
        windows_[name] = std::move(window);
}

void VideoWindows::reopenOutputs() {
    for (const auto& name : windowfloat::outputsToReopen(host_.model(), "Screen")) open(name);
}

void VideoWindows::closed(const std::string& name) {
    juce::MessageManager::callAsync([this, alive = alive_, name] {
        if (*alive) windows_.erase(name);
    });
}

}
