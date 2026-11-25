// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/browser/Audition.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <juce_audio_formats/juce_audio_formats.h>

#include "core/app/AppPaths.h"
#include "core/browser/FileKind.h"
#include "gui/video/VideoProbe.h"

namespace hum::browser {

namespace {

std::shared_ptr<const PreviewSound> silenceAsLongAsThePicture(const std::string& path) {
    const double seconds = std::min(probeVideoSeconds(fileAt(path)), Audition::kLongestSeconds);
    if (seconds <= 0.0) return nullptr;
    auto sound = std::make_shared<PreviewSound>();
    sound->sampleRate = Audition::kSilentRate;
    sound->left.assign((size_t) std::ceil(seconds * Audition::kSilentRate) + 1, 0.0f);
    sound->right = sound->left;
    return sound;
}

std::shared_ptr<const PreviewSound> decode(const std::string& path) {
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(fileAt(path)));
    if (!reader || reader->sampleRate <= 0.0 || reader->lengthInSamples <= 1)
        return kindOfFile(path) == Kind::Video ? silenceAsLongAsThePicture(path) : nullptr;
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
    const auto stamp = fileAt(path).getLastModificationTime().toMilliseconds();
    {
        const std::lock_guard<std::mutex> hold(lock_);
        current_ = path;
        fileBpm_ = kindOfFile(path) == Kind::Video ? 0.0 : fileBpm;
        if (sound_ != nullptr && soundPath_ == path && soundStamp_ == stamp) {
            applySpeed();
            voice_.play(sound_);
            return;
        }
    }
    pool_.addJob([this, path, mine, stamp] {
        if (request_.load() != mine) return;
        auto sound = decode(path);
        decodes_.fetch_add(1);
        if (request_.load() != mine) return;
        const std::lock_guard<std::mutex> hold(lock_);
        if (current_ != path) return;
        sound_ = sound;
        soundPath_ = path;
        soundStamp_ = stamp;
        applySpeed();
        voice_.play(sound);
    });
}

void Audition::stop() {
    ++request_;
    voice_.stop();
}

void Audition::toggle() {
    if (voice_.playing() || voice_.paused()) {
        voice_.setPaused(!voice_.holding());
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
