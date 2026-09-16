// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "SoundIn/SoundIn.h"

#include <algorithm>
#include <utility>
#include <memory>

#include <juce_audio_formats/juce_audio_formats.h>

namespace hum {

void SoundIn::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    meter_.prepare(sampleRate);
    loop_ = params.get("Loop", 1.0) >= 0.5;
    gainSm_ = (float) params.get("Gain", 1.0);
    readPos_ = 0;
    loadedUri_.clear();
    loadFromFile(params.getText("File"));
    if (hasPending_.load()) applyPending();
}

void SoundIn::loadFromFile(const std::string& uriIn) {
    if (uriIn == loadedUri_) return;
    loadedUri_ = uriIn;
    juce::AudioBuffer<float> buf;
    std::string uri = uriIn;
    if (!uri.empty()) {
        if (uri.rfind("file://", 0) == 0) uri = uri.substr(7);
        juce::File f(juce::String(juce::CharPointer_UTF8(uri.c_str())));
        if (f.existsAsFile()) {
            juce::AudioFormatManager fm;
            fm.registerBasicFormats();
            std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(f));
            if (reader) {
                buf.setSize((int) reader->numChannels, (int) reader->lengthInSamples);
                reader->read(&buf, 0, (int) reader->lengthInSamples, 0, true, true);
            }
        }
    }
    {
        const juce::ScopedLock sl(loadLock_);
        pending_ = std::move(buf);
    }
    hasPending_.store(true);
}

void SoundIn::applyPending() {
    const juce::ScopedTryLock sl(loadLock_);
    if (!sl.isLocked()) return;
    std::swap(file_, pending_);
    readPos_ = 0;
    hasPending_.store(false);
}

void SoundIn::process(const float* const*, int,
                      float* const* out, int numOut,
                      int numSamples, const Transport& transport) {
    if (hasPending_.load()) applyPending();
    loop_ = params.get("Loop", 1.0) >= 0.5;
    const bool wantADC = params.get("UseADC", 1.0) >= 0.5;
    const bool haveLive = transport.liveInputChannels() > 0;
    if (wantADC && haveLive) {
        const int base = channel();
        for (int c = 0; c < numOut; ++c) {
            const float* src = transport.liveInput(base + c);
            if (!src) src = transport.liveInput(base);
            if (src) std::copy(src, src + numSamples, out[c]);
            else std::fill(out[c], out[c] + numSamples, 0.0f);
        }
    } else {
        const int fileCh = file_.getNumChannels();
        const int64_t len = file_.getNumSamples();
        if (fileCh == 0 || len == 0) {
            for (int c = 0; c < numOut; ++c) std::fill(out[c], out[c] + numSamples, 0.0f);
        } else {
            for (int n = 0; n < numSamples; ++n) {
                int64_t pos = readPos_ + n;
                if (pos >= len) {
                    if (loop_) pos %= len;
                    else { for (int c = 0; c < numOut; ++c) out[c][n] = 0.0f; continue; }
                }
                for (int c = 0; c < numOut; ++c) {
                    int src = std::min(c, fileCh - 1);
                    out[c][n] = file_.getSample(src, (int) pos);
                }
            }
            readPos_ += numSamples;
            if (loop_ && len > 0) readPos_ %= len;
        }
    }

    meter_.measure(out, numOut, numSamples);
    applyGain(out, numOut, numSamples);
}

void SoundIn::applyGain(float* const* out, int numOut, int numSamples) {
    const float target = (float) std::clamp(params.get("Gain", 1.0), 0.0, 2.0);
    if (target == 1.0f && gainSm_ == 1.0f) return;
    const float step = (target - gainSm_) / (float) std::max(1, numSamples);
    for (int n = 0; n < numSamples; ++n) {
        gainSm_ += step;
        for (int c = 0; c < numOut; ++c) out[c][n] *= gainSm_;
    }
    gainSm_ = target;
}

}
