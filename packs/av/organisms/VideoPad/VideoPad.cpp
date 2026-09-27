// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "VideoPad/VideoPad.h"

#include <algorithm>
#include <cstddef>
#include <cmath>

#include <cstdlib>

#include "hum/Registry.h"
#include "hum/dsp/SoundFileBuffer.h"

namespace hum {

namespace {
constexpr const char* kLaunchParams[VideoPadSource::kMaxClips] = {
    "Launch1", "Launch2", "Launch3", "Launch4",
    "Launch5", "Launch6", "Launch7", "Launch8"};
constexpr const char* kFileParams[VideoPadSource::kMaxClips] = {
    "File1", "File2", "File3", "File4", "File5", "File6", "File7", "File8"};
static_assert(VideoPadSource::kMaxClips == 8, "the name tables list one param per pad");

int slotOf(const std::string& param, const char* prefix) {
    const std::string p(prefix);
    if (param.size() != p.size() + 1 || param.compare(0, p.size(), p) != 0) return -1;
    const int n = param.back() - '1';
    return n >= 0 && n < VideoPadSource::kMaxClips ? n : -1;
}
}

void VideoPad::reset() {
    pos_.fill(0.0);
    for (auto& h : heard_) h.store(0.0, std::memory_order_relaxed);
    primed_ = false;
    lastStop_ = false;
    pendingMidiCount_ = 0;
    for (auto& l : lastLaunch_) l = false;
    pendingLaunch_.store(-1, std::memory_order_relaxed);
    active_.store(-1, std::memory_order_relaxed);
    outgoing_.store(-1, std::memory_order_relaxed);
    phase_.store(1.0f, std::memory_order_relaxed);
}

bool VideoPad::loaded(int slot) const {
    return slot >= 0 && slot < padCount() && !params.getText(kFileParams[slot]).empty();
}

void VideoPad::launch(int slot) {
    if (slot < 0 || slot >= kMaxClips) return;
    pos_[(size_t) slot] = 0.0;
    heard_[(size_t) slot].store(0.0, std::memory_order_relaxed);
    const int current = active_.load(std::memory_order_relaxed);
    const float fade = (float) params.get("Fade", 0.5);
    if (slot != current) {
        outgoing_.store(current, std::memory_order_relaxed);
        active_.store(slot, std::memory_order_relaxed);
        phase_.store(fade > 0.0f ? 0.0f : 1.0f, std::memory_order_relaxed);
    }
    launches_.fetch_add(1, std::memory_order_relaxed);
}

void VideoPad::stop() {
    const int current = active_.load(std::memory_order_relaxed);
    if (current < 0) return;
    const float fade = (float) params.get("Fade", 0.5);
    outgoing_.store(current, std::memory_order_relaxed);
    active_.store(-1, std::memory_order_relaxed);
    phase_.store(fade > 0.0f ? 0.0f : 1.0f, std::memory_order_relaxed);
}

bool VideoPad::liveParamRange(const std::string& param, double& lo, double& hi) const {
    if (const int s = slotOf(param, "In"); s >= 0) {
        lo = 0.0;
        hi = lengths_[(size_t) s].load(std::memory_order_relaxed);
        return true;
    }
    if (const int s = slotOf(param, "Out"); s >= 0) {
        const double len = lengths_[(size_t) s].load(std::memory_order_relaxed);
        lo = std::min(len, params.get(std::string("In") + (char) ('1' + s), 0.0));
        hi = len;
        return true;
    }
    return false;
}

bool VideoPad::rangeFollowsFile(const std::string& param, const std::string& fileParam) const {
    return !param.empty() && !fileParam.empty() && param.back() == fileParam.back();
}

void VideoPad::rolled() {
    int candidates[kMaxClips];
    int n = 0;
    for (int i = 0; i < padCount(); ++i)
        if (loaded(i)) candidates[n++] = i;
    if (n == 0) return;
    pendingLaunch_.store(candidates[std::rand() % n], std::memory_order_relaxed);
}

void VideoPad::deliverMidi(int, const MidiEvent* events, int count) {
    for (int i = 0; i < count; ++i) {
        const auto& e = events[i];
        if (e.size < 3 || (e.data[0] & 0xF0) != 0x90 || e.data[2] == 0) continue;
        if (pendingMidiCount_ >= kMaxClips) break;
        const int rel = (int) e.data[1] - kBaseNote;
        const int pads = padCount();
        pendingMidi_[pendingMidiCount_++] = ((rel % pads) + pads) % pads;
    }
}

void VideoPad::loadFromFile(const std::string&) {
    for (int slot = 0; slot < kMaxClips; ++slot) {
        const auto uri = resolvePath(params.getText("File" + std::to_string(slot + 1)), "VideoPad");
        if (uri == uri_[(size_t) slot]) continue;
        uri_[(size_t) slot] = uri;
        std::shared_ptr<Clip> fresh;
        if (!uri.empty()) {
            fresh = std::make_shared<Clip>();
            if (!loadSoundBytes(uri, *fresh)) fresh.reset();
            else if (fresh->sampleRate <= 0.0) fresh->sampleRate = sampleRate_;
        }
        srcRate_[(size_t) slot].store(fresh != nullptr ? fresh->sampleRate : 0.0,
                                      std::memory_order_relaxed);
        const std::lock_guard<std::mutex> sl(clipLock_);
        clips_[(size_t) slot] = std::move(fresh);
    }
}

void VideoPad::readClip(int slot, float* const* out, int numSamples, float gain) {
    if (slot < 0 || slot >= kMaxClips) return;
    std::shared_ptr<const Clip> clip;
    {
        std::unique_lock<std::mutex> sl(clipLock_, std::try_to_lock);
        if (!sl.owns_lock()) return;
        clip = clips_[(size_t) slot];
    }
    if (clip == nullptr || clip->sampleRate <= 0.0 || clip->empty()) return;

    const int frames = clip->frames();
    const int chans = clip->chans();

    const auto n = std::to_string(slot + 1);
    const double in = std::max(0.0, params.get("In" + n, 0.0)) * clip->sampleRate;
    const double outAt = params.get("Out" + n, 0.0) * clip->sampleRate;
    const double last = outAt > in ? std::min(outAt, (double) frames) : (double) frames;
    const bool loops = params.get("Loop" + n, 1.0) >= 0.5;
    const double step = clip->sampleRate / std::max(1.0, sampleRate_)
                      * std::max(0.0, params.get("Rate", 1.0));

    double at = pos_[(size_t) slot];
    if (at < in) at = in;
    for (int i = 0; i < numSamples; ++i) {
        if (at >= last) {
            if (!loops) break;
            at = in + std::fmod(at - in, std::max(1.0, last - in));
        }
        const int j = (int) at;
        const float frac = (float) (at - (double) j);
        const int k = j + 1 < frames ? j + 1 : j;
        for (int c = 0; c < 2; ++c) {
            const auto& src = clip->channels[(std::size_t) (c < chans ? c : chans - 1)];
            out[c][i] += gain * (src[j] + frac * (src[k] - src[j]));
        }
        at += step;
    }
    pos_[(size_t) slot] = at;
    heard_[(size_t) slot].store(at, std::memory_order_relaxed);
}

void VideoPad::process(const float* const*, int, float* const* out, int numOut, int numSamples,
                       const Transport&) {
    bool launchNow[kMaxClips];
    for (int i = 0; i < kMaxClips; ++i)
        launchNow[i] = params.get(kLaunchParams[i], 0.0) >= 0.5;
    const bool stopNow = params.get("Stop", 0.0) >= 0.5;
    if (!primed_) {
        primed_ = true;
        for (int i = 0; i < kMaxClips; ++i) lastLaunch_[i] = launchNow[i];
        lastStop_ = stopNow;
    }
    for (int i = 0; i < kMaxClips; ++i) {
        if (launchNow[i] && !lastLaunch_[i]) launch(i);
        lastLaunch_[i] = launchNow[i];
    }
    for (int i = 0; i < pendingMidiCount_; ++i) launch(pendingMidi_[i]);
    pendingMidiCount_ = 0;
    if (const int slot = pendingLaunch_.exchange(-1, std::memory_order_relaxed); slot >= 0)
        launch(slot);
    if (stopNow && !lastStop_) stop();
    lastStop_ = stopNow;

    float phase = phase_.load(std::memory_order_relaxed);
    const bool muted = params.get("Mute", 0.0) >= 0.5;
    const int active = active_.load(std::memory_order_relaxed);
    const int outgoing = outgoing_.load(std::memory_order_relaxed);
    for (int c = 0; c < numOut; ++c)
        if (out[c] != nullptr) std::fill(out[c], out[c] + numSamples, 0.0f);
    if (!muted && numOut >= 2 && out != nullptr) {
        readClip(active, out, numSamples, phase);
        if (phase < 1.0f) readClip(outgoing, out, numSamples, 1.0f - phase);
    }

    if (phase < 1.0f) {
        const double fade = std::max(0.0, params.get("Fade", 0.5));
        const double step = fade <= 0.0 ? 1.0 : (double) numSamples / (fade * sampleRate_);
        phase = (float) std::min(1.0, (double) phase + step);
        phase_.store(phase, std::memory_order_relaxed);
        if (phase >= 1.0f) outgoing_.store(-1, std::memory_order_relaxed);
    }
}

}
