// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Leafcutter/Leafcutter.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "hum/dsp/RexFile.h"
#include "hum/dsp/SoundFileBuffer.h"

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {
constexpr double kFadeSeconds = 0.003;

double loadRexLoop(const std::string& uri, juce::AudioBuffer<float>& buf,
                   double& sr, std::vector<SliceOnset>& onsets, int& mapLen) {
    std::string path = uri;
    if (path.rfind("file://", 0) == 0) path = path.substr(7);
    const juce::File f(juce::String(juce::CharPointer_UTF8(path.c_str())));
    const RexData rex = loadRexFile(f);
    if (!rex.parsed) {
        loadSoundFile(path, buf, sr);
        return 0.0;
    }

    if (rex.hasAudio) {
        buf = rex.audio;
        sr = rex.sampleRate > 0.0 ? rex.sampleRate : sr;
    } else {
        for (const char* ext : {".wav", ".aif", ".aiff", ".flac"}) {
            const auto sib = f.getSiblingFile(
                f.getFileNameWithoutExtension() + ext);
            if (sib.existsAsFile()
                && loadSoundFile(sib.getFullPathName().toStdString(), buf, sr))
                break;
        }
    }
    const int len = buf.getNumSamples();
    mapLen = len > 0 ? len : rex.totalSamples;
    if (mapLen > 0 && rex.totalSamples > 0) {
        const double scale = (double) mapLen / (double) rex.totalSamples;
        for (const int pos : rex.slices) {
            const int p = (int) std::lround(pos * scale);
            if (p < 0 || p >= mapLen) continue;
            if ((int) onsets.size() >= kMaxSliceOnsets) break;
            onsets.push_back({p, onsets.empty() ? 1.0f : 0.995f});
        }
    }
    return rex.bars > 0 && rex.beatsPerBar > 0
               ? (double) (rex.bars * rex.beatsPerBar) : 0.0;
}
}

void Leafcutter::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    loadFromFile({});
    applyPending();
    syncEdits();
    reset();
}

void Leafcutter::reset() {
    for (auto& v : voices_) v = Voice{};
    loopVoice_ = Voice{};
    lastWindow_ = -1;
    lastLoopBeat_ = -1.0;
    beat_ = 0.0;
    stagedCount_ = 0;
    playing_.store(-1, std::memory_order_relaxed);
}

void Leafcutter::loadFromFile(const std::string&) {
    const std::string uri = params.getText("File");
    if (uri == uri_) return;
    uri_ = uri;
    juce::AudioBuffer<float> buf;
    double sr = sampleRate_;
    std::vector<SliceOnset> fileOnsets;
    double fileBeats = 0.0;
    int rexMapLen = 0;
    if (!uri.empty()) {
        if (isRexPath(uri))
            fileBeats = loadRexLoop(uri, buf, sr, fileOnsets, rexMapLen);
        else
            loadSoundFile(uri, buf, sr);
    }

    const int n = buf.getNumSamples();
    std::vector<float> mono((size_t) std::max(0, n), 0.0f);
    for (int c = 0; c < buf.getNumChannels(); ++c) {
        const float* src = buf.getReadPointer(c);
        const float g = 1.0f / (float) buf.getNumChannels();
        for (int i = 0; i < n; ++i) mono[(size_t) i] += src[i] * g;
    }
    std::vector<SliceOnset> onsets =
        !fileOnsets.empty() ? fileOnsets
        : n > 0             ? detectSliceOnsets(mono.data(), n, sr)
                            : std::vector<SliceOnset>{};

    peaks_.assign(512, 0.0f);
    for (int i = 0; i < n; ++i) {
        auto& bin = peaks_[(size_t) ((juce::int64) i * 512 / std::max(1, n))];
        bin = std::max(bin, std::abs(mono[(size_t) i]));
    }
    guiOnsets_ = onsets;
    guiLen_ = n > 0 ? n : rexMapLen;
    gen_.fetch_add(1, std::memory_order_relaxed);

    juce::AudioBuffer<float> retiredBuf;
    std::vector<SliceOnset> retiredOnsets;
    {
        const juce::SpinLock::ScopedLockType sl(swap_);
        retiredBuf = std::move(pendingBuf_);
        retiredOnsets = std::move(pendingOnsets_);
        pendingBuf_ = std::move(buf);
        pendingRate_ = sr > 0.0 ? sr : sampleRate_;
        pendingOnsets_ = std::move(onsets);
        pendingFileBeats_ = fileBeats;
        hasPending_.store(true, std::memory_order_release);
    }
}

void Leafcutter::applyPending() {
    if (!hasPending_.load(std::memory_order_acquire)) return;
    const juce::SpinLock::ScopedTryLockType sl(swap_);
    if (!sl.isLocked()) return;
    std::swap(buf_, pendingBuf_);
    srcRate_ = pendingRate_;
    onsets_.swap(pendingOnsets_);
    fileBeats_ = pendingFileBeats_;
    hasPending_.store(false, std::memory_order_relaxed);
    lastSense_ = -1.0;
    lastPermCount_ = -1;
    for (auto& v : voices_) v = Voice{};
    loopVoice_ = Voice{};
    lastWindow_ = -1;
}

std::vector<float> Leafcutter::sliceStarts() const {
    std::vector<float> out;
    if (guiLen_ <= 0) return out;
    std::vector<SliceOnset> act;
    activeSliceOnsets(guiOnsets_, params.get("Sense", 0.5), act);
    out.reserve(act.size());
    for (const auto& o : act) out.push_back((float) o.pos / (float) guiLen_);
    return out;
}

void Leafcutter::deliverMidi(int, const MidiEvent* events, int count) {
    stagedCount_ = std::min(count, (int) staged_.size());
    std::copy(events, events + stagedCount_, staged_.begin());
}

void Leafcutter::pushLiveMidi(const MidiEvent& e) {
    std::lock_guard<std::mutex> lock(liveLock_);
    if (liveCount_ < (int) liveQ_.size()) liveQ_[(size_t) liveCount_++] = e;
}

void Leafcutter::triggerVoice(Voice& v, int slice, double windowOutSamples, float gain) {
    const int len = buf_.getNumSamples();
    if (slice < 0 || slice >= activeCount() || len <= 0) { v.active = false; return; }
    SliceEdit se;
    if (const auto it = edits_.find(slice); it != edits_.end()) se = it->second;
    const double start = active_[(size_t) slice].pos;
    const double end = slice + 1 < activeCount() ? (double) active_[(size_t) slice + 1].pos
                                                 : (double) len;
    const double gate = std::clamp(params.get("Gate", 1.0), 0.05, 1.0);
    const double pitch = std::clamp(params.get("Pitch", 0.0) + se.pitch, -48.0, 48.0);
    const bool reverse = (params.get("Reverse", 0.0) >= 0.5) != se.reverse;
    const double base = (srcRate_ / sampleRate_) * std::pow(2.0, pitch / 12.0);
    v.start = start;
    v.end = end;
    v.step = reverse ? -base : base;
    v.pos = reverse ? std::max(start, end - 2.0) : start;
    v.gateLeft = std::max(1.0, windowOutSamples * gate);
    v.gain = gain * (float) se.gainPct / 100.0f;
    v.env = 0.0f;
    v.decayGain = 1.0f;
    v.releasing = false;
    v.active = true;
}

void Leafcutter::handleEvent(const MidiEvent& e) {
    if (e.size < 3) return;
    const int status = e.data[0] & 0xF0;
    const int note = e.data[1];
    const int vel = e.data[2];
    if (status == 0x80 || (status == 0x90 && vel == 0)) {
        for (auto& v : voices_)
            if (v.active && v.note == note) v.releasing = true;
        return;
    }
    if (status != 0x90 || activeCount() == 0) return;
    const int slice = ((note - kBaseNote) % activeCount() + activeCount()) % activeCount();
    Voice* pick = nullptr;
    for (auto& v : voices_) if (!v.active) { pick = &v; break; }
    if (pick == nullptr) pick = &voices_[0];
    const double start = active_[(size_t) slice].pos;
    const double end = slice + 1 < activeCount() ? (double) active_[(size_t) slice + 1].pos
                                                 : (double) buf_.getNumSamples();
    const double sliceOut = (end - start) / srcRate_ * sampleRate_;
    triggerVoice(*pick, slice, sliceOut, (float) vel / kMidiMaxF);
    pick->note = note;
    lastMidiSlice_ = slice;
}

void Leafcutter::renderAdd(float* left, float* right, int from, int count) {
    const int len = buf_.getNumSamples();
    if (len < 2) return;
    const float* srcL = buf_.getReadPointer(0);
    const float* srcR = buf_.getNumChannels() > 1 ? buf_.getReadPointer(1) : srcL;
    const float fade = (float) (1.0 / (kFadeSeconds * sampleRate_));
    const float level = (float) std::clamp(params.get("Level", 1.0), 0.0, 2.0);
    const double decay = std::clamp(params.get("Decay", 0.0), 0.0, 1.0);
    const float decayCoef = decay <= 0.0
        ? 1.0f
        : (float) std::exp(-1.0 / (sampleRate_ * (0.6 * (1.0 - decay) + 0.04)));

    auto renderVoice = [&](Voice& v) {
        if (!v.active) return;
        for (int i = from; i < from + count && v.active; ++i) {
            const bool ranOut = v.step >= 0.0 ? v.pos >= v.end - 1.0 : v.pos <= v.start;
            if (ranOut || (v.releasing && v.env <= 0.0f)) { v.active = false; break; }
            const bool falling = v.releasing || v.gateLeft < 1.0 / fade;
            v.env = falling ? std::max(0.0f, v.env - fade) : std::min(1.0f, v.env + fade);
            v.gateLeft -= 1.0;
            if (v.gateLeft <= 0.0) v.releasing = true;
            v.decayGain *= decayCoef;
            const int i0 = std::min((int) v.pos, len - 2);
            const float frac = (float) (v.pos - i0);
            const float g = v.env * v.gain * v.decayGain * level;
            left[i] += (srcL[i0] + (srcL[i0 + 1] - srcL[i0]) * frac) * g;
            if (right != nullptr)
                right[i] += (srcR[i0] + (srcR[i0 + 1] - srcR[i0]) * frac) * g;
            v.pos += v.step;
        }
    };
    renderVoice(loopVoice_);
    for (auto& v : voices_) renderVoice(v);
}

void Leafcutter::process(const float* const*, int, float* const* out, int numOut,
                         int numSamples, const Transport& transport) {
    if (numOut < 1) return;
    applyPending();
    pendingEdits_.adopt(edits_);
    float* left = out[0];
    float* right = numOut > 1 ? out[1] : nullptr;
    std::memset(left, 0, sizeof(float) * (size_t) numSamples);
    if (right != nullptr) std::memset(right, 0, sizeof(float) * (size_t) numSamples);
    for (int c = 2; c < numOut; ++c)
        std::memset(out[c], 0, sizeof(float) * (size_t) numSamples);

    {
        std::unique_lock<std::mutex> lock(liveLock_, std::try_to_lock);
        if (lock.owns_lock()) {
            for (int i = 0; i < liveCount_ && stagedCount_ < (int) staged_.size(); ++i)
                staged_[(size_t) stagedCount_++] = liveQ_[(size_t) i];
            liveCount_ = 0;
        }
    }

    const double sense = std::clamp(params.get("Sense", 0.5), 0.0, 1.0);
    if (sense != lastSense_) {
        activeSliceOnsets(onsets_, sense, active_);
        lastSense_ = sense;
        lastWindow_ = -1;
    }
    const int shuffle = (int) params.get("Shuffle", 0.0);
    if (shuffle != lastShuffle_ || activeCount() != lastPermCount_) {
        slicePermutation(activeCount(), shuffle, perm_.data());
        lastShuffle_ = shuffle;
        lastPermCount_ = activeCount();
        lastWindow_ = -1;
    }

    for (int i = 0; i < stagedCount_; ++i) handleEvent(staged_[(size_t) i]);
    stagedCount_ = 0;

    const int len = buf_.getNumSamples();
    const bool play = params.get("Play", 1.0) >= 0.5 && len > 0 && activeCount() > 0;
    const double tempo = transport.tempo();
    const double bps = tempo / kSecondsPerMinute / sampleRate_;
    double beat = transport.playing() ? transport.beats() : beat_;

    double beatsTotal = params.get("Beats", 0.0);
    if (beatsTotal < 0.5 && fileBeats_ > 0.5)
        beatsTotal = fileBeats_;
    else if (beatsTotal < 0.5 && len > 0)
        beatsTotal = std::round((double) len / srcRate_ * tempo / kSecondsPerMinute);
    beatsTotal = std::clamp(beatsTotal, 1.0, 64.0);
    const double spb = sampleRate_ * kSecondsPerMinute / tempo;

    if (play) {
        int done = 0;
        while (done < numSamples) {
            const double lb = std::fmod(beat, beatsTotal);
            int w = activeCount() - 1;
            for (int k = 1; k < activeCount(); ++k)
                if ((double) active_[(size_t) k].pos / len * beatsTotal > lb) { w = k - 1; break; }
            const double endB = w + 1 < activeCount()
                                    ? (double) active_[(size_t) w + 1].pos / len * beatsTotal
                                    : beatsTotal;
            if (w != lastWindow_ || lb < lastLoopBeat_) {
                const double startB = (double) active_[(size_t) w].pos / len * beatsTotal;
                triggerVoice(loopVoice_, perm_[(size_t) w], (endB - startB) * spb, 1.0f);
                loopVoice_.note = -1;
                lastWindow_ = w;
            }
            lastLoopBeat_ = lb;
            const int toBoundary = (int) std::ceil((endB - lb) / bps);
            const int seg = std::clamp(toBoundary, 1, numSamples - done);
            renderAdd(left, right, done, seg);
            beat += bps * seg;
            done += seg;
        }
    } else {
        loopVoice_.active = false;
        lastWindow_ = -1;
        beat += bps * numSamples;
        renderAdd(left, right, 0, numSamples);
    }
    beat_ = beat;

    bool midiActive = false;
    for (const auto& v : voices_)
        if (v.active) { midiActive = true; break; }
    playing_.store(midiActive ? lastMidiSlice_ : (play ? lastWindow_ : -1),
                   std::memory_order_relaxed);
}

}
