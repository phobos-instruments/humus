// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/decks/DeckModels.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace hum::decks {

void DeckPad::hotCue(int index, bool clear) {
    if (clear) decks_.clearHotCue(organism_, p_, index);
    else if (host_.liveParamValue(organism_, p_.hotCue(index)) > 0.0) decks_.jumpHotCue(organism_, p_, index);
    else decks_.setHotCue(organism_, p_, index);
}

void PitchModel::cycleRange() {
    const double cur = host_.liveParamValue(organism_, range_);
    host_.setParam(organism_, range_, cur < 12.0 ? 16.0 : cur < 30.0 ? 50.0 : 8.0);
}

DeckViewModel::Window DeckViewModel::window() const {
    Window w;
    w.length = decks_.length(organism_);
    w.rate = decks_.sampleRate(organism_);
    w.position = decks_.position(organism_);
    w.span = std::max(1.0, viewSeconds() * w.rate);
    w.left = (double) w.position - w.span * 0.5;
    return w;
}

std::string DeckViewModel::zoomText() const {
    const double seconds = viewSeconds();
    return seconds < 1.0 ? decimalText(seconds, 1) + " s" : std::to_string((int) seconds) + " s";
}

std::vector<std::pair<double, bool>> DeckViewModel::beatLines(const Window& w) const {
    std::vector<std::pair<double, bool>> out;
    const auto g = grid();
    if (g.samplesPerBeat(w.rate) <= 1.0) return out;
    for (double bk = std::floor(g.sampleToBeat(w.left, w.rate));; bk += 1.0) {
        const double s = g.beatToSample(bk, w.rate);
        if (s > w.left + w.span) break;
        if (s < 0 || s > (double) w.length) continue;
        out.emplace_back(s, std::fmod(std::fabs(bk), 4.0) < 0.001);
    }
    return out;
}

std::string DeckViewModel::bpmText() const {
    const double eff = decks_.effectiveBpm(organism_);
    return eff > 0.0 ? decimalText(eff, 1) : std::string();
}

double PitchModel::drivenPercent() const {
    if (bpm_.empty()) return pitch();
    const double grid = host_.liveParamValue(organism_, bpm_);
    const double eff = decks_.effectiveBpm(organism_);
    if (grid <= 0.0 || eff <= 0.0) return pitch();
    return (eff / grid - 1.0) * 100.0;
}

std::string PitchModel::drivenText() const {
    const double driven = drivenPercent();
    if (std::abs(driven - pitch()) < 0.05) return {};
    return (driven >= 0 ? "+" : "") + decimalText(driven, 1) + "%";
}

std::string DeckViewModel::infoText() const {
    std::string info = host_.liveParamText(organism_, p_.key);
    if (value(p_.quantize) >= 0.5) info += "  Q";
    return info;
}

bool DeckViewModel::isAudioPath(const std::string& path) {
    std::string l = path;
    for (auto& c : l) c = (char) std::tolower((unsigned char) c);
    for (const char* ext : {".wav", ".aiff", ".aif", ".flac", ".mp3", ".ogg"}) {
        const std::string e(ext);
        if (l.size() >= e.size() && l.compare(l.size() - e.size(), e.size(), e) == 0) return true;
    }
    return false;
}

std::int64_t DeckViewModel::sampleAt(int x, int x0, int width) const {
    const auto w = window();
    const double s = w.left + (double) (x - x0) / std::max(1, width) * w.span;
    return (std::int64_t) std::clamp(s, 0.0, (double) w.length);
}

void DeckViewModel::seekOverview(int x, int x0, int width) {
    const std::int64_t len = decks_.length(organism_);
    const double frac = std::clamp((double) (x - x0) / std::max(1, width), 0.0, 1.0);
    decks_.seek(organism_, (std::int64_t) (frac * (double) len));
}

void DeckViewModel::beginScrub(int x) {
    scrubFrom_ = decks_.position(organism_);
    host_.ensureAudio();
    scrubX_ = x;
    decks_.setScrub(organism_, true, (double) scrubFrom_);
}

void DeckViewModel::scrubTo(int x, int width) {
    const double rate = decks_.sampleRate(organism_);
    const double spp = std::max(1.0, viewSeconds() * rate) / std::max(1, width);
    const std::int64_t len = decks_.length(organism_);
    const double np = (double) scrubFrom_ - (double) (x - scrubX_) * spp;
    decks_.setScrub(organism_, true, std::clamp(np, 0.0, (double) len));
}

void DeckViewModel::loopTo(std::int64_t sample) {
    host_.setParam(organism_, p_.loopIn, (double) std::min(loopAnchor_, sample));
    host_.setParam(organism_, p_.loopOut, (double) std::max(loopAnchor_, sample));
}

}
