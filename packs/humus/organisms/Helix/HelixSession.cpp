// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Helix/Helix.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>

#include "hum/dsp/Interpolation.h"
#include "hum/dsp/SoundFileBuffer.h"

namespace hum {

bool Helix::storeSessionAudio(const std::string& pathPrefix,
                              std::vector<std::pair<std::string, std::string>>& out) {
    bool any = false;
    for (int t = 0; t < kStrands; ++t) {
        auto& s = strands_[(size_t) t];
        const std::string param = "Loop" + std::to_string(t + 1);
        if (s.len <= 0) {
            if (!params.getText(param).empty()) {
                out.push_back({param, {}});
                any = true;
            }
            continue;
        }
        const std::string path = pathPrefix + "-loop" + std::to_string(t + 1) + ".wav";
        if (!writeStrand(s, path)) continue;
        out.push_back({param, path});
        s.loadedUri = path;
        any = true;
    }
    return any;
}

bool Helix::audioTakeReady(int take) const {
    if (take < 0 || take >= kStrands) return false;
    const auto& s = strands_[(size_t) take];
    return s.len > 0 && s.state != SState::Rec;
}

bool Helix::writeAudioTake(int take, const std::string& wavPath) {
    return audioTakeReady(take) && writeStrand(strands_[(size_t) take], wavPath);
}

bool Helix::writeStrand(const Strand& s, const std::string& wavPath) const {
    juce::AudioBuffer<float> b(2, (int) s.len);
    for (int c = 0; c < 2; ++c)
        b.copyFrom(c, 0, s.buf[(size_t) c].data(), (int) s.len);
    return writeSoundFile(wavPath, b, sampleRate_, -1, 24, s.recBpm);
}

void Helix::loadSessionAudio() {
    for (int t = 0; t < kStrands; ++t) {
        auto& s = strands_[(size_t) t];
        if (maxLoopSamples_ <= 0) continue;
        const auto uri = params.getText("Loop" + std::to_string(t + 1));
        if (uri.empty() || (uri == s.loadedUri && s.state != SState::Empty)) continue;
        if (s.state == SState::Rec || s.state == SState::Dub) continue;
        if (s.state != SState::Empty) clearStrand(s);
        juce::AudioBuffer<float> b;
        SoundFileInfo info;
        if (!loadSoundFile(uri, b, info) || b.getNumSamples() <= 0) continue;
        const double ratio = info.sampleRate > 0.0 ? sampleRate_ / info.sampleRate : 1.0;
        const auto len = std::min<std::int64_t>(
            maxLoopSamples_, (std::int64_t) std::llround((double) b.getNumSamples() * ratio));
        if (len <= 0) continue;
        const int src = b.getNumSamples();
        for (int c = 0; c < 2; ++c) {
            const float* from = b.getReadPointer(std::min(c, b.getNumChannels() - 1));
            for (std::int64_t i = 0; i < len; ++i) {
                const double x = (double) i / ratio;
                const int a = std::min((int) x, src - 1);
                const int a1 = std::min(a + 1, src - 1);
                const float fr = (float) (x - (double) a);
                s.buf[(size_t) c][(size_t) i] = from[a] * (1.0f - fr) + from[a1] * fr;
            }
        }
        s.pos = 0.0;
        s.recBpm = info.tempoBpm;
        s.len = len;
        s.layers = 1;
        s.loadedUri = uri;
        s.state = SState::Stopped;
        s.rebuildAt = 0;
        s.uiState.store((int) SState::Stopped, std::memory_order_relaxed);
        s.uiLayers.store(1, std::memory_order_relaxed);
    }
}

void Helix::markPeak(Strand& s, std::int64_t at, float l, float r) {
    const std::int64_t c = at / kPeakChunk;
    if (c < 0 || c >= (std::int64_t) s.chunkPeak.size()) return;
    if (c != s.peakChunkAt) {
        s.peakChunkAt = c;
        s.chunkPeak[(size_t) c] = 0.0f;
    }
    const float v = std::max(std::fabs(l), std::fabs(r));
    if (v > s.chunkPeak[(size_t) c]) s.chunkPeak[(size_t) c] = v;
    s.waveDirty = true;
}

void Helix::rebuildPeaks(Strand& s) {
    if (s.rebuildAt < 0) return;
    const std::int64_t reach = s.len > 0 ? s.len : s.recCount;
    const std::int64_t chunks = reach / kPeakChunk;
    const std::int64_t stop = std::min(chunks, s.rebuildAt + kRebuildChunks);
    for (std::int64_t c = s.rebuildAt; c < stop && c < (std::int64_t) s.chunkPeak.size(); ++c) {
        const std::int64_t from = c * kPeakChunk;
        const std::int64_t to = std::min(from + kPeakChunk, reach);
        float peak = 0.0f;
        for (std::int64_t i = from; i < to; ++i)
            peak = std::max(peak, std::max(std::fabs(s.buf[0][(size_t) i]),
                                           std::fabs(s.buf[1][(size_t) i])));
        s.chunkPeak[(size_t) c] = peak;
    }
    s.rebuildAt = stop >= chunks ? -1 : stop;
    s.waveDirty = true;
}

void Helix::publishWave(Strand& s) {
    const std::int64_t reach = s.state == SState::Rec ? s.recCount : s.len;
    const std::int64_t chunks = reach / kPeakChunk;
    if (chunks <= 0) {
        s.uiWaveBins.store(0, std::memory_order_relaxed);
        return;
    }
    const int bins = StrandWave::kWaveBins;
    float loudest = 0.0f;
    for (int b = 0; b < bins; ++b) {
        const std::int64_t from = chunks * b / bins;
        const std::int64_t to = std::max(from + 1, chunks * (b + 1) / bins);
        float peak = 0.0f;
        for (std::int64_t c = from; c < to && c < (std::int64_t) s.chunkPeak.size(); ++c)
            peak = std::max(peak, s.chunkPeak[(size_t) c]);
        s.uiWave[(size_t) b].store(peak, std::memory_order_relaxed);
        loudest = std::max(loudest, peak);
    }
    if (loudest > 1.0e-4f)
        for (int b = 0; b < bins; ++b)
            s.uiWave[(size_t) b].store(s.uiWave[(size_t) b].load(std::memory_order_relaxed) / loudest,
                                       std::memory_order_relaxed);
    s.uiWaveBins.store(bins, std::memory_order_relaxed);
}

int Helix::strandWave(int strand, float* out, int max) const {
    if (strand < 0 || strand >= kStrands || out == nullptr || max <= 0) return 0;
    const auto& s = strands_[(size_t) strand];
    const int bins = std::min(max, s.uiWaveBins.load(std::memory_order_relaxed));
    for (int b = 0; b < bins; ++b) out[b] = s.uiWave[(size_t) b].load(std::memory_order_relaxed);
    return bins;
}

float Helix::strandInput(int strand) const {
    return strand >= 0 && strand < kStrands
               ? strands_[(size_t) strand].uiInput.load(std::memory_order_relaxed) : 0.0f;
}

void Helix::publishUi() {
    for (auto& s : strands_) {
        rebuildPeaks(s);
        s.inShow = std::max(s.inPeak, s.inShow * 0.82f);
        s.inPeak = 0.0f;
        s.uiInput.store(s.inShow, std::memory_order_relaxed);
        if (s.waveDirty) {
            publishWave(s);
            s.waveDirty = false;
        }
        const bool running = s.len > 0 && (s.state == SState::Play || s.state == SState::Dub);
        s.uiState.store((int) (s.dubTail >= 0 ? SState::Play : s.state),
                        std::memory_order_relaxed);
        s.uiPhase.store(running ? (float) (s.pos / (double) s.len) : -1.0f,
                        std::memory_order_relaxed);
        s.uiLayers.store(s.layers, std::memory_order_relaxed);
        s.uiPending.store(s.pending != Pending::None || s.revPend >= 0 || s.halfPend >= 0,
                          std::memory_order_relaxed);
        const bool settled = s.state == SState::Play || s.state == SState::Stopped;
        s.uiPendingPress.store(pressKind(s), std::memory_order_relaxed);
        s.uiCanUndo.store(s.state == SState::Rec || s.state == SState::Dub || (settled && s.undoReady && !s.redo),
                          std::memory_order_relaxed);
        s.uiCanRedo.store(settled && s.undoReady && s.redo, std::memory_order_relaxed);
    }
}

int Helix::strandState(int strand) const {
    return strand >= 0 && strand < kStrands
               ? strands_[(size_t) strand].uiState.load(std::memory_order_relaxed) : 0;
}

float Helix::strandPhase(int strand) const {
    return strand >= 0 && strand < kStrands
               ? strands_[(size_t) strand].uiPhase.load(std::memory_order_relaxed) : -1.0f;
}

int Helix::strandLayers(int strand) const {
    return strand >= 0 && strand < kStrands
               ? strands_[(size_t) strand].uiLayers.load(std::memory_order_relaxed) : 0;
}

bool Helix::strandPending(int strand) const {
    return strand >= 0 && strand < kStrands
           && strands_[(size_t) strand].uiPending.load(std::memory_order_relaxed);
}

int Helix::pressKind(const Strand& s) {
    switch (s.pending) {
        case Pending::RecordPress: return kPressRecord;
        case Pending::DubPress: return kPressDub;
        case Pending::PlayPress: return kPressPlay;
        case Pending::StopPress: return kPressStop;
        case Pending::RecPress:
            if (s.state == SState::Play || s.state == SState::Dub) return kPressDub;
            return s.state == SState::Stopped ? kPressPlay : kPressRecord;
        case Pending::None: break;
    }
    return kPressNone;
}

int Helix::strandPendingPress(int strand) const {
    return strand >= 0 && strand < kStrands
               ? strands_[(size_t) strand].uiPendingPress.load(std::memory_order_relaxed) : kPressNone;
}

bool Helix::strandCanUndo(int strand) const {
    return strand >= 0 && strand < kStrands
           && strands_[(size_t) strand].uiCanUndo.load(std::memory_order_relaxed);
}

bool Helix::strandCanRedo(int strand) const {
    return strand >= 0 && strand < kStrands
           && strands_[(size_t) strand].uiCanRedo.load(std::memory_order_relaxed);
}

}
