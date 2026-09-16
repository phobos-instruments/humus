// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Graft/Graft.h"

#include <algorithm>
#include <cmath>
#include <sstream>

#include <juce_events/juce_events.h>

#include "hum/dsp/SoundFileBuffer.h"

namespace hum {

namespace {

constexpr double kDivisionBeats[] = {4.0,       2.0,       1.0,       0.5,
                                     0.25,      0.125,     4.0 / 3.0, 2.0 / 3.0,
                                     1.0 / 3.0, 1.0 / 6.0, 1.5,       0.75};
constexpr int kDivisions = (int) (sizeof(kDivisionBeats) / sizeof(kDivisionBeats[0]));
constexpr double kFallbackTempo = 120.0;
constexpr int kOnsetsPerSource = 32;

bool decodePin(const std::string& payload, GraftSlot& slot) {
    std::istringstream fields(payload);
    std::string field;
    int v[3] = {0, 0, 0};
    int n = 0;
    bool ok = true;
    while (ok && n < 3 && std::getline(fields, field, ':')) ok = parseSignedInt(field, v[n++]);
    if (!ok || n != 3 || v[0] < 0 || v[0] >= kGraftSlots) return false;
    slot.source = v[0];
    slot.fragment = std::max(0, v[1]);
    slot.reverse = v[2] != 0;
    return true;
}

std::string encodePin(const GraftSlot& slot) {
    return std::to_string(slot.source) + ":" + std::to_string(slot.fragment) + ":"
           + (slot.reverse ? "1" : "0");
}

std::map<int, GraftSlot> parsePins(const std::string& text) {
    std::map<int, GraftSlot> out;
    for (const auto& [index, payload] : parseSlicePins(text))
        if (GraftSlot slot; decodePin(payload, slot)) out[index] = slot;
    return out;
}

int wrapped(int value, int count) {
    return count > 0 ? ((value % count) + count) % count : 0;
}

}

bool Graft::reloadsOn(const std::string& param) const {
    return param == "Slices" || param == "Order" || param == "Cut" || param == "Div"
           || param == "Length" || param == "Sense" || param == "Xfade" || param == "Seed"
           || param == "Bars" || param == "Normalize" || param == "Source";
}

GraftSpec Graft::specFromParams(double, double tempo) const {
    GraftSpec spec;
    spec.slices = (int) std::clamp(params.get("Slices", 16.0), 4.0, (double) kGraftMaxSlices);
    spec.order = (GraftOrder) (int) std::clamp(params.get("Order", 0.0), 0.0, 5.0);
    spec.cut = (GraftCut) (int) std::clamp(params.get("Cut", 0.0), 0.0, 2.0);
    spec.lengthPct = std::clamp(params.get("Length", 100.0), 10.0, 200.0);
    spec.xfadeMs = std::clamp(params.get("Xfade", 6.0), 0.0, 50.0);
    spec.normalise = params.get("Normalize", 1.0) >= 0.5;
    spec.seed = (int) std::clamp(params.get("Seed", 0.0), 0.0, 999.0);
    spec.one = (int) std::clamp(params.get("Source", 1.0), 1.0, (double) kGraftSlots) - 1;
    spec.bars = (int) std::clamp(params.get("Bars", 0.0), 0.0, 16.0);

    const double bpm = tempo > 0.0 ? tempo : kFallbackTempo;
    const double secondsPerBeat = kSecondsPerMinute / bpm;
    const int division = (int) std::clamp(params.get("Div", 4.0), 0.0, (double) (kDivisions - 1));
    spec.gridSeconds = kDivisionBeats[division] * secondsPerBeat;
    spec.barSeconds = lastBeatsPerBar_.load(std::memory_order_relaxed) * secondsPerBeat;
    return spec;
}

bool Graft::refreshSources() {
    bool changed = false;
    for (int i = 0; i < kGraftSlots; ++i) {
        const std::string uri = params.getText("File" + std::to_string(i + 1));
        if (uri == uris_[(size_t) i]) continue;
        uris_[(size_t) i] = uri;
        changed = true;
        GraftSource& s = sources_[(size_t) i];
        s = GraftSource{};
        s.rate = sampleRate_;
        detected_[(size_t) i].clear();
        if (uri.empty()) continue;

        double rate = sampleRate_;
        if (!loadSoundFile(uri, s.buf, rate) || s.buf.getNumSamples() < 2) {
            s.buf.setSize(0, 0);
            continue;
        }
        s.rate = rate > 0.0 ? rate : sampleRate_;
        const int n = s.buf.getNumSamples();
        s.mono.assign((size_t) n, 0.0f);
        for (int c = 0; c < s.buf.getNumChannels(); ++c) {
            const float* src = s.buf.getReadPointer(c);
            const float g = 1.0f / (float) s.buf.getNumChannels();
            for (int j = 0; j < n; ++j) s.mono[(size_t) j] += src[j] * g;
        }
        detected_[(size_t) i] = detectSliceOnsets(s.mono.data(), n, s.rate);
    }

    const double sense = std::clamp(params.get("Sense", 0.5), 0.0, 1.0);
    std::vector<SliceOnset> active;
    for (int i = 0; i < kGraftSlots; ++i) {
        GraftSource& s = sources_[(size_t) i];
        s.cuts.clear();
        if (!s.ready()) continue;
        activeSliceOnsets(detected_[(size_t) i], sense, active);
        for (const auto& onset : active) {
            if ((int) s.cuts.size() >= kOnsetsPerSource) break;
            s.cuts.push_back(onset.pos);
        }
        if (s.cuts.empty() || s.cuts.front() != 0) s.cuts.insert(s.cuts.begin(), 0);
    }
    return changed;
}

void Graft::syncTextParams() {
    if (const std::string text = params.getText("SliceEdits"); text != appliedEdits_) {
        appliedEdits_ = text;
        edits_ = parseSliceEdits(text);
    }
    if (const std::string text = params.getText("Pins"); text != appliedPins_) {
        appliedPins_ = text;
        pins_ = parsePins(text);
    }
}

void Graft::restitchNow() {
    stopTimer();
    refreshSources();
    syncTextParams();
    restitch(sampleRate_);
}

void Graft::timerCallback() { restitchNow(); }

void Graft::loadFromFile(const std::string& uri) {
    if (!uri.empty() || buf_.getNumSamples() < 2) { restitchNow(); return; }
    startTimer(kCoalesceMs);
}

void Graft::restitch(double destRate) {
    const GraftSpec spec = specFromParams(destRate, lastTempo_.load(std::memory_order_relaxed));
    graftPlan(sources_, spec, pins_, plan_);
    for (int i = 0; i < (int) plan_.size(); ++i) {
        const auto it = edits_.find(i);
        if (it == edits_.end()) continue;
        GraftSlot& slot = plan_[(size_t) i];
        slot.pitch = it->second.pitch;
        slot.gainPct = it->second.gainPct;
        slot.reverse = slot.reverse != it->second.reverse;
    }
    publish(graftStitch(sources_, plan_, spec, destRate));
}

void Graft::publish(GraftResult&& result) {
    stretch_ = result.stretch;
    const int n = result.buf.getNumSamples();
    guiSeconds_ = sampleRate_ > 0.0 ? (double) n / sampleRate_ : 0.0;

    peaks_.assign((size_t) kPeakBins, 0.0f);
    if (n > 0) {
        const float* src = result.buf.getReadPointer(0);
        for (int i = 0; i < n; ++i) {
            float& bin = peaks_[(size_t) ((juce::int64) i * kPeakBins / n)];
            bin = std::max(bin, std::abs(src[i]));
        }
    }
    guiBounds_ = result.bounds;
    if (!guiBounds_.empty()) guiBounds_.pop_back();
    gen_.fetch_add(1, std::memory_order_relaxed);

    const int loaded = (int) graftLoaded(sources_).size();
    juce::String line;
    line << "creature " << (int) params.get("Seed", 0.0) << "   " << (int) plan_.size()
         << " slices   "
         << juce::String(guiSeconds_, 2) << " s   " << loaded
         << (loaded == 1 ? " source" : " sources");
    if (const int bars = (int) params.get("Bars", 0.0); bars > 0) {
        line << "   " << bars << (bars == 1 ? " bar" : " bars");
        const double shift = -kSemitonesPerOctave * std::log2(std::max(1.0e-6, result.stretch));
        if (std::abs(shift) >= 0.05)
            line << "  " << (shift > 0.0 ? "+" : "") << juce::String(shift, 1) << " st";
    }
    if (loaded == 0) line = "drop sound files into the slots above";
    status_ = line.toStdString();

    juce::AudioBuffer<float> retired;
    {
        const juce::SpinLock::ScopedLockType lock(swap_);
        retired = std::move(pendingBuf_);
        pendingBuf_ = std::move(result.buf);
        pendingBounds_ = result.bounds;
        hasPending_.store(true, std::memory_order_release);
    }
}

std::string Graft::pinValue(int index) const {
    if (index < 0 || index >= (int) plan_.size()) return {};
    return encodePin(plan_[(size_t) index]);
}

std::string Graft::pinAlternative(int index) const {
    const std::vector<int> loaded = graftLoaded(sources_);
    if (loaded.empty()) return {};
    const GraftSpec spec = specFromParams(sampleRate_,
                                          lastTempo_.load(std::memory_order_relaxed));
    auto& dice = juce::Random::getSystemRandom();
    GraftSlot slot;
    slot.source = loaded[(size_t) dice.nextInt((int) loaded.size())];
    slot.fragment = dice.nextInt(std::max(1, graftFragments(sources_[(size_t) slot.source], spec)));
    slot.reverse = index >= 0 && index < (int) plan_.size() && plan_[(size_t) index].reverse;
    return encodePin(slot);
}

std::string Graft::pinNudged(const std::string& from, int sourceStep, int fragmentStep) const {
    GraftSlot slot;
    const std::vector<int> loaded = graftLoaded(sources_);
    if (loaded.empty() || !decodePin(from, slot)) return {};
    const GraftSpec spec = specFromParams(sampleRate_,
                                          lastTempo_.load(std::memory_order_relaxed));
    const auto at = std::find(loaded.begin(), loaded.end(), slot.source);
    const int place = at == loaded.end() ? 0 : (int) (at - loaded.begin());
    slot.source = loaded[(size_t) wrapped(place + sourceStep, (int) loaded.size())];
    const int count = graftFragments(sources_[(size_t) slot.source], spec);
    slot.fragment = wrapped(slot.fragment + fragmentStep, count);
    return encodePin(slot);
}

std::string Graft::originName(int origin) const {
    if (origin < 0 || origin >= kGraftSlots || uris_[(size_t) origin].empty()) return {};
    std::string path = uris_[(size_t) origin];
    if (path.rfind("file://", 0) == 0) path = path.substr(7);
    return juce::File(juce::String(juce::CharPointer_UTF8(path.c_str())))
        .getFileNameWithoutExtension()
        .toStdString();
}

int Graft::sliceOrigin(int index) const {
    return index >= 0 && index < (int) plan_.size() ? plan_[(size_t) index].source : -1;
}

int Graft::textLines(std::string* out, int capacity) const {
    if (out == nullptr || capacity < 1 || status_.empty()) return 0;
    out[0] = status_;
    return 1;
}

}
