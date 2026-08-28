#include "gui/EngineHost.h"

#include <cstdint>
#include <vector>

#include "hum/Capabilities.h"
#include "hum/dsp/SoundFileBuffer.h"
#include "core/BankLibrary.h"
#include "core/BeatDetector.h"
#include "hum/dsp/BeatGrid.h"
#include "core/KeyDetector.h"

namespace hum {

std::int64_t DeckHost::position(const std::string& name) {
    if (host_.graph_)
        if (auto* d = dynamic_cast<DeckControl*>(host_.graph_->find(name)))
            return d->playbackPositionSamples();
    return 0;
}

std::int64_t DeckHost::length(const std::string& name) {
    if (host_.graph_)
        if (auto* d = dynamic_cast<DeckControl*>(host_.graph_->find(name)))
            return d->fileLengthSamples();
    return 0;
}

double DeckHost::sampleRate(const std::string& name) {
    if (host_.graph_)
        if (auto* d = dynamic_cast<DeckControl*>(host_.graph_->find(name)))
            return d->playbackSampleRate();
    return host_.sampleRate_;
}

double DeckHost::effectiveBpm(const std::string& name) {
    if (host_.graph_)
        if (auto* d = dynamic_cast<DeckControl*>(host_.graph_->find(name)))
            return d->effectiveBpm();
    return 0.0;
}

void DeckHost::seek(const std::string& name, std::int64_t sample) {
    if (host_.graph_)
        if (auto* d = dynamic_cast<DeckControl*>(host_.graph_->find(name)))
            d->requestSeekSamples(sample);
}

void DeckHost::setBend(const std::string& name, double percent) {
    if (host_.graph_)
        if (auto* d = dynamic_cast<DeckControl*>(host_.graph_->find(name)))
            d->setBendPercent(percent);
}

void DeckHost::setScrub(const std::string& name, bool active, double targetSample) {
    if (host_.graph_)
        if (auto* d = dynamic_cast<DeckControl*>(host_.graph_->find(name)))
            d->setScrub(active, targetSample);
}

std::vector<float> DeckHost::waveform(const std::string& name) {
    if (host_.graph_)
        if (auto* d = dynamic_cast<DeckControl*>(host_.graph_->find(name)))
            return d->waveformPeaks();
    return {};
}

std::int64_t DeckHost::quantizeToGrid(const std::string& name, std::int64_t sample) {
    if (host_.liveParamValue(name, "Quantize") < 0.5) return sample;
    BeatGrid g{std::max(1.0, host_.liveParamValue(name, "BPM")),
               (std::int64_t) host_.liveParamValue(name, "GridOffset")};
    const double sr = sampleRate(name);
    return (std::int64_t) std::max(0.0, g.nearestBeatSample((double) sample, sr));
}

void DeckHost::setCue(const std::string& name) {
    host_.setParam(name, "Cue", (double) quantizeToGrid(name, position(name)));
}
void DeckHost::jumpCue(const std::string& name) {
    seek(name, (std::int64_t) host_.liveParamValue(name, "Cue"));
}
void DeckHost::setHotCue(const std::string& name, int index) {
    if (index >= 1 && index <= 8)
        host_.setParam(name, "HotCue_" + std::to_string(index),
                       (double) quantizeToGrid(name, position(name)));
}
void DeckHost::jumpHotCue(const std::string& name, int index) {
    if (index < 1 || index > 8) return;
    const double v = host_.liveParamValue(name, "HotCue_" + std::to_string(index));
    if (v > 0.0) seek(name, (std::int64_t) v);
}
void DeckHost::clearHotCue(const std::string& name, int index) {
    if (index >= 1 && index <= 8) host_.setParam(name, "HotCue_" + std::to_string(index), -1.0);
}
void DeckHost::setBeatLoop(const std::string& name, double beats) {
    const double bpm = std::max(1.0, host_.liveParamValue(name, "BPM"));
    const double spb = 60.0 / bpm * sampleRate(name);
    const std::int64_t in = quantizeToGrid(name, position(name));
    host_.setParam(name, "LoopBeats", beats);
    host_.setParam(name, "LoopIn", (double) in);
    host_.setParam(name, "LoopOut", (double) in + beats * spb);
    host_.setParam(name, "Loop", 1.0);
}
void DeckHost::scaleBeatLoop(const std::string& name, double factor) {
    const double beats = juce::jlimit(0.0625, 64.0, host_.liveParamValue(name, "LoopBeats") * factor);
    const double in = host_.liveParamValue(name, "LoopIn");
    const double spb = 60.0 / std::max(1.0, host_.liveParamValue(name, "BPM")) * sampleRate(name);
    host_.setParam(name, "LoopBeats", beats);
    host_.setParam(name, "LoopOut", in + beats * spb);
}
void DeckHost::toggleLoop(const std::string& name) {
    host_.setParam(name, "Loop", host_.liveParamValue(name, "Loop") >= 0.5 ? 0.0 : 1.0);
}
void DeckHost::beginLoopRoll(const std::string& name, double beats) {
    EngineHost::LiveControlScope live(host_);
    setBeatLoop(name, beats);
    if (host_.graph_)
        if (auto* d = dynamic_cast<DeckControl*>(host_.graph_->find(name))) d->beginLoopRoll();
}
void DeckHost::endLoopRoll(const std::string& name) {
    EngineHost::LiveControlScope live(host_);
    if (host_.graph_)
        if (auto* d = dynamic_cast<DeckControl*>(host_.graph_->find(name))) d->endLoopRoll();
    host_.setParam(name, "Loop", 0.0);
}

void EngineHost::onFileNodeChanged(const std::string& organism, const std::string& text,
                                   bool sourceMoved) {
    Organism* c = graph_ ? graph_->find(organism) : nullptr;
    const auto* cm = model_.byName(organism);
    const auto path = cm != nullptr ? banks::resolve(text, cm->displayClass) : text;
    if (auto* fl = dynamic_cast<FileLoader*>(c)) fl->loadFromFile(path);
    if (auto* live = dynamic_cast<LiveParamRange*>(c);
        sourceMoved && live != nullptr && cm != nullptr) {
        std::vector<std::pair<std::string, double>> restarts;
        for (const auto& p : cm->properties) {
            double lo = 0.0, hi = 0.0;
            if (live->liveParamRange(p.name, lo, hi)) restarts.emplace_back(p.name, lo);
        }
        for (const auto& [n, v] : restarts) setParam(organism, n, v);
    }
    pullVoiceParams(organism);
    autoDetectDeckGrid(organism, path);
}

void EngineHost::autoDetectDeckGrid(const std::string& organism, const std::string& filePath) {
    if (filePath.empty()) return;
    if (!graph_ || !dynamic_cast<DeckControl*>(graph_->find(organism))) return;
    juce::AudioBuffer<float> tmp;
    double sr = sampleRate_;
    if (!loadSoundFile(filePath, tmp, sr)) return;
    const auto est = detectBeat(tmp, sr);
    if (est.confidence > 0.0) {
        setParam(organism, "BPM", est.bpm);
        setParam(organism, "GridOffset", (double) est.offsetSamples);
    }
    const auto key = detectKey(tmp, sr);
    if (key.pitchClass >= 0)
        setParamText(organism, "Key", key.name + "  " + key.camelot);
}

}
