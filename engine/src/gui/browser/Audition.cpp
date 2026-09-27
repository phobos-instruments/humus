// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/browser/Audition.h"

#include <algorithm>
#include <cstdint>

#include <juce_audio_formats/juce_audio_formats.h>

#include "core/app/AppPaths.h"

namespace hum::browser {

namespace {

std::shared_ptr<const PreviewSound> decode(const std::string& path) {
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(fileAt(path)));
    if (!reader || reader->sampleRate <= 0.0 || reader->lengthInSamples <= 1) return nullptr;
    const auto frames = (int) std::min<std::int64_t>(reader->lengthInSamples,
                                                     (std::int64_t) (reader->sampleRate * Audition::kLongestSeconds));
    juce::AudioBuffer<float> buf((int) std::max(1u, std::min(2u, reader->numChannels)), frames);
    reader->read(&buf, 0, frames, 0, true, true);
    auto sound = std::make_shared<PreviewSound>();
    sound->sampleRate = reader->sampleRate;
    sound->left.assign(buf.getReadPointer(0), buf.getReadPointer(0) + frames);
    const int right = buf.getNumChannels() > 1 ? 1 : 0;
    sound->right.assign(buf.getReadPointer(right), buf.getReadPointer(right) + frames);
    return sound;
}

}

Audition::~Audition() {
    pool_.removeAllJobs(true, 4000);
    voice_.stop();
}

void Audition::play(const std::string& path, double fileBpm) {
    const unsigned mine = ++request_;
    {
        const std::lock_guard<std::mutex> hold(lock_);
        current_ = path;
        fileBpm_ = fileBpm;
    }
    pool_.addJob([this, path, mine] {
        if (request_.load() != mine) return;
        auto sound = decode(path);
        if (request_.load() != mine) return;
        const std::lock_guard<std::mutex> hold(lock_);
        if (current_ != path) return;
        sound_ = sound;
        applySpeed();
        voice_.play(sound);
    });
}

void Audition::stop() {
    ++request_;
    voice_.stop();
}

void Audition::toggle() {
    if (voice_.playing()) {
        stop();
        return;
    }
    const auto path = current();
    double bpm = 0.0;
    {
        const std::lock_guard<std::mutex> hold(lock_);
        bpm = fileBpm_;
    }
    if (!path.empty()) play(path, bpm);
}

void Audition::setSync(bool on) {
    const std::lock_guard<std::mutex> hold(lock_);
    sync_ = on;
    applySpeed();
}

void Audition::applySpeed() {
    const double tempo = tempo_ ? tempo_() : 0.0;
    voice_.setSpeed(sync_ && fileBpm_ > 0.0 && tempo > 0.0 ? tempo / fileBpm_ : 1.0);
}

std::string Audition::current() const {
    const std::lock_guard<std::mutex> hold(lock_);
    return current_;
}

double Audition::length() const {
    const std::lock_guard<std::mutex> hold(lock_);
    return sound_ != nullptr && sound_->sampleRate > 0.0 ? (double) sound_->frames() / sound_->sampleRate : 0.0;
}

bool Audition::waitIdle(int ms) {
    const auto until = juce::Time::getMillisecondCounter() + (juce::uint32) ms;
    while (pool_.getNumJobs() > 0) {
        if (juce::Time::getMillisecondCounter() > until) return false;
        juce::Thread::sleep(2);
    }
    return true;
}

}
