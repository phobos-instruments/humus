// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "AudioTrack/AudioTrack.h"

#include "hum/dsp/SoundFileBuffer.h"

namespace hum {

std::shared_ptr<const ClipPcm> AudioTrack::fileFor(const std::string& path) {
    if (const auto it = cache_.find(path); it != cache_.end()) return it->second;
    auto pcm = loadClipPcm(path, sampleRate_);
    if (pcm != nullptr) cache_.emplace(path, pcm);
    return pcm;
}

void AudioTrack::ensureClipsLoaded() {
    if (!clipsDirty_) return;
    clipsDirty_ = false;

    std::vector<ClipVoice> next;
    for (int i = 0; i < (int) pattern_.channels.size(); ++i) {
        const auto& ch = pattern_.channels[(size_t) i];
        if (ch.type != "audio-clip" || ch.startTick < 0 || ch.audioFile.empty()) continue;
        if (const auto pcm = fileFor(ch.audioFile)) appendClipVoices(next, pattern_, i, pcm, sampleRate_);
    }
    {
        const juce::ScopedLock sl(loadLock_);
        pendingClips_.swap(next);
    }
    hasPending_.store(true, std::memory_order_release);
}

}
