// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/video/VideoModels.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

namespace hum::video {

std::string VideoTransportModel::clock(double seconds) {
    const int total = (int) std::floor(std::max(0.0, seconds));
    const int secs = total % 60;
    return std::to_string(total / 60) + ":" + (secs < 10 ? "0" : "") + std::to_string(secs);
}

void VideoTransportModel::stop(PlaybackTarget* t) {
    if (t == nullptr) return;
    t->setPaused(true);
    t->seekSeconds(0.0);
}

VideoTransportModel::Shown VideoTransportModel::shown(PlaybackTarget* t) {
    Shown s;
    if (!live(t)) return s;
    s.live = true;
    const double len = t->lengthSeconds();
    const double pos = std::clamp(t->positionSeconds(), 0.0, len);
    s.fraction = len > 0.0 ? pos / len : 0.0;
    s.time = clock(pos) + " / " + clock(len);
    s.paused = t->isPaused() ? 1 : 0;
    return s;
}

bool CamPreviewModel::active() const {
    auto* src = source();
    return src != nullptr && src->camActive();
}

std::string CamPreviewModel::message(bool stalled) const {
    auto* src = source();
    if (src == nullptr) return say(host_, kNoLiveInstance);
    if (const auto blocked = src->camUnavailable(); !blocked.empty()) return blocked;
    if (src->camActive() && !stalled) return {};
    return stalled ? std::string("no picture arriving - is the source on?") : say(host_, kCameraOff);
}

bool CamPreviewModel::frameChanged(bool stalled) {
    auto* src = source();
    const unsigned gen = src ? src->camGeneration() : 0;
    const bool on = src && src->camActive();
    const bool nowStalled = on && stalled;
    if (gen == lastGen_ && on == lastActive_ && nowStalled == wasStalled_) return false;
    wasStalled_ = nowStalled;
    lastGen_ = gen;
    lastActive_ = on;
    return true;
}

bool CamPreviewModel::producesVideo() const {
    auto* vn = live::source<VideoNode>(host_, organism_);
    return vn != nullptr && vn->numVideoOutputs() > 0;
}

CamPreviewModel::Signals CamPreviewModel::signals() const {
    Signals s;
    if (auto* src = source()) src->camSignals(s.x, s.y, s.motion, s.bright);
    return s;
}

}
