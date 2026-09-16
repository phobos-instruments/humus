// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"
#include "core/packs/Categories.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "hum/CameraCapture.h"
#include "hum/SerialPort.h"
#include "core/graph/GraphIo.h"
#include "core/net/LinkChase.h"
#include "core/packs/Roles.h"
#include "gui/app/AppSettings.h"
#include "io/PatchLoader.h"
#include "io/WavWriter.h"
#include "hum/caps/Audio.h"
#include "hum/dsp/DspMath.h"

namespace hum {

bool EngineHost::openOfflineSound(OfflineSound& made, std::string& error) {
    made.graph = std::make_unique<AudioGraph>();
    if (!buildGraph(model_, *made.graph, error)) {
        made.graph.reset();
        return false;
    }
    made.graph->prepare(sampleRate_, block_, model_.clock.tempo);
    primeNoteTracks(*made.graph);
    for (int i = 0; i < made.graph->nodeCount(); ++i)
        if (auto* s = dynamic_cast<MasterTap*>(made.graph->organism(i))) { made.tap = s; break; }
    if (made.tap == nullptr) {
        error = "patch has no SoundOut";
        made.graph.reset();
        return false;
    }
    return true;
}

bool EngineHost::renderOfflineSound(OfflineSound& made, double fromSeconds, double toSeconds,
                                    const SoundSink& sink, std::string& error) {
    if (made.graph == nullptr || made.tap == nullptr) { error = "nothing to render"; return false; }
    if (toSeconds <= fromSeconds) { error = "that range is empty"; return false; }
    const juce::ScopedNoDenormals noDenormals;
    auto& g = *made.graph;
    auto* so = made.tap;
    const int64_t skip = (int64_t) (std::max(0.0, fromSeconds) * sampleRate_);
    const int64_t total = (int64_t) (toSeconds * sampleRate_);
    const int channels = so->channels();
    std::vector<const float*> ptrs((size_t) channels);
    int64_t done = 0;
    while (done < total) {
        const int n = (int) std::min<int64_t>(block_, total - done);
        g.processBlock(n);
        const int madeNow = so->lastBlockLength();
        const int64_t from = std::max<int64_t>(0, skip - done);
        if (from < madeNow) {
            for (int c = 0; c < channels; ++c) ptrs[(size_t) c] = so->channelData(c) + from;
            if (!sink(ptrs.data(), channels, madeNow - (int) from)) {
                error = "the bounce was stopped";
                return false;
            }
        }
        done += n;
    }
    return true;
}

bool EngineHost::renderRange(double fromSeconds, double toSeconds, const SoundSink& sink,
                             std::string& error) {
    OfflineSound made;
    return openOfflineSound(made, error)
           && renderOfflineSound(made, fromSeconds, toSeconds, sink, error);
}

bool EngineHost::renderToFile(const std::string& path, double seconds, std::string& error) {
    std::vector<std::vector<float>> buf;
    const bool made = renderRange(0.0, seconds,
                                  [&buf](const float* const* in, int channels, int n) {
        if (buf.empty()) buf.resize((size_t) channels);
        for (int c = 0; c < channels; ++c)
            buf[(size_t) c].insert(buf[(size_t) c].end(), in[c], in[c] + n);
        return true;
    }, error);
    if (!made) return false;
    if (!writeSound(path, buf, sampleRate_)) { error = "could not write " + path; return false; }
    return true;
}

bool EngineHost::startMixRecording(const std::string& path, std::string& error) {
    const juce::ScopedLock sl(lock_);
    if (mixWriter_.active()) mixWriter_.stop();
    const double sr = sampleRate_ > 0.0 ? sampleRate_ : kDefaultSampleRate;
    if (!mixWriter_.start(path, 2, sr, false)) {
        error = "could not open " + path + " for recording";
        return false;
    }
    mixRecording_.store(true);
    return true;
}

void EngineHost::stopMixRecording() {
    const juce::ScopedLock sl(lock_);
    mixRecording_.store(false);
    mixWriter_.stop();
}

void EngineHost::primeOffline(int blocks) {
    const juce::ScopedLock sl(lock_);
    if (!graph_ || blocks <= 0) return;
    const juce::ScopedNoDenormals noDenormals;
    takeTransportRequests();
    const std::int64_t pos = graph_->transport().samplePosition();
    const double beats = graph_->transport().beats();
    for (int i = 0; i < blocks; ++i) graph_->processBlock(block_);
    graph_->transport().restorePosition(pos, beats);
    publishClock();
}

}
