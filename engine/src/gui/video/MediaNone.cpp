// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include <memory>

#include "gui/video/VideoEncoder.h"
#include "gui/video/VideoLayer.h"

namespace hum {

std::unique_ptr<VideoLayer> VideoLayer::create(bool) { return nullptr; }
std::unique_ptr<VideoLayer> VideoLayer::createPlatform() { return nullptr; }
std::unique_ptr<VideoLayer> VideoLayer::createOffline() { return nullptr; }
std::unique_ptr<VideoLayer> VideoLayer::createFfmpeg() { return nullptr; }
double VideoLayer::probeLengthSeconds(const juce::File&) { return 0.0; }
bool VideoLayer::systemCanPlay(const juce::File&) { return false; }

VideoThread::VideoThread() = default;
VideoThread::~VideoThread() = default;

bool videoKindAvailable(VideoKind) { return false; }
bool videoLogIsLoud() { return false; }
const char* videoKindExtension(VideoKind kind) { return kind == VideoKind::Hap ? "mov" : "mp4"; }

std::unique_ptr<VideoEncoder> makeVideoWriter(VideoKind, const juce::File&, int, int, double, int, bool) {
    return nullptr;
}

}
