// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstddef>
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <new>
#include <string>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>

#include "hum/dsp/DspMath.h"

namespace hum {

struct SoundFileInfo {
    double sampleRate = kDefaultSampleRate;
    int rootKey = -1;
    int loopStart = 0, loopEnd = 0;
    double tempoBpm = 0.0;
};

using SoundFileProgress = std::function<void(float)>;

using SoundFileAbort = std::function<bool()>;

using SharedAudio = std::shared_ptr<const juce::AudioBuffer<float>>;

struct SharedSound {
    SharedAudio audio;
    SoundFileInfo info;
};

using SoundFill = std::shared_ptr<std::atomic<std::int64_t>>;

using SoundFileFilling = std::function<void(std::int64_t filled, std::int64_t total)>;

using SoundFileOpened = std::function<void(const SharedSound& sound, const SoundFill& filled)>;

inline constexpr double kSoundReadMargin = 8.0;

inline bool soundReadyFor(double at, std::int64_t ready, int samples, double rate) {
    if (ready <= 0) return false;
    const double reach = at + (double) samples * (rate < 0.0 ? -rate : rate) + kSoundReadMargin;
    return reach < (double) ready;
}

using ExtraSoundFormats = std::function<std::vector<std::unique_ptr<juce::AudioFormat>>(int track)>;

using SoundFileTrackNames = std::function<std::vector<std::string>(const std::string& uri)>;

inline SoundFileTrackNames& soundFileTrackNames() {
    static SoundFileTrackNames names;
    return names;
}

inline std::vector<std::string> soundFileTracks(const std::string& uri) {
    auto& names = soundFileTrackNames();
    return names ? names(uri) : std::vector<std::string>{};
}

inline std::atomic<unsigned>& soundFileReads() {
    static std::atomic<unsigned> reads{0};
    return reads;
}

inline ExtraSoundFormats& extraSoundFormats() {
    static ExtraSoundFormats formats;
    return formats;
}

inline bool loadSoundFile(std::string uri, juce::AudioBuffer<float>& dest,
                          SoundFileInfo& info, const SoundFileProgress& onProgress = {},
                          double maxSeconds = 0.0, const SoundFileAbort& abandon = {},
                          int track = 0, const SoundFileFilling& filling = {}) {
    dest.setSize(0, 0);
    info = SoundFileInfo{};
    if (uri.empty()) return false;
    if (uri.rfind("file://", 0) == 0) uri = uri.substr(7);
    juce::File f(juce::String(juce::CharPointer_UTF8(uri.c_str())));
    if (!f.existsAsFile()) return false;

    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    if (auto& extra = extraSoundFormats())
        for (auto& format : extra(track)) fm.registerFormat(format.release(), false);
    std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(f));
    if (!reader || reader->lengthInSamples <= 0) return false;

    const std::int64_t wanted = maxSeconds > 0.0
        ? std::min<std::int64_t>(reader->lengthInSamples,
                                 (std::int64_t) (maxSeconds * std::max(1.0, reader->sampleRate)))
        : (std::int64_t) reader->lengthInSamples;
    if (wanted <= 0 || wanted > (std::int64_t) std::numeric_limits<int>::max()) return false;
    try {
        dest.setSize((int) reader->numChannels, (int) wanted);
    } catch (const std::bad_alloc&) {
        dest.setSize(0, 0);
        return false;
    }
    const int total = (int) wanted;
    info.sampleRate = reader->sampleRate > 0.0 ? reader->sampleRate : kDefaultSampleRate;
    ++soundFileReads();
    if (!onProgress && !abandon && !filling) {
        reader->read(&dest, 0, total, 0, true, true);
    } else {
        const int chunk = (int) std::max(1.0, reader->sampleRate);
        for (int at = 0; at < total; at += chunk) {
            if (abandon && abandon()) {
                if (!filling) dest.setSize(0, 0);
                return false;
            }
            const int take = std::min(chunk, total - at);
            reader->read(&dest, at, take, at, true, true);
            if (filling) filling(at + take, total);
            if (onProgress)
                onProgress((float) std::min(1.0, (double) (at + chunk) / (double) total));
        }
    }

    const auto& meta = reader->metadataValues;
    if (meta.containsKey("MidiUnityNote"))
        info.rootKey = juce::jlimit(0, kMidiMax, meta["MidiUnityNote"].getIntValue());
    if (meta.getValue("NumSampleLoops", "0").getIntValue() > 0) {
        info.loopStart = meta["Loop0Start"].getIntValue();
        info.loopEnd = meta["Loop0End"].getIntValue();
    }
    if (meta.containsKey(juce::WavAudioFormat::acidTempo))
        info.tempoBpm = juce::jmax(0.0, meta[juce::WavAudioFormat::acidTempo].getDoubleValue());
    const auto len = total;
    info.loopStart = juce::jlimit(0, len - 1, info.loopStart);
    info.loopEnd = juce::jlimit(0, len - 1, info.loopEnd);
    return true;
}

struct SoundBytes {
    std::vector<std::vector<float>> channels;
    double sampleRate = 0.0;

    int chans() const { return (int) channels.size(); }
    int frames() const { return channels.empty() ? 0 : (int) channels.front().size(); }
    bool empty() const { return frames() <= 0 || chans() <= 0; }
};

inline bool loadSoundBytes(const std::string& uri, SoundBytes& out) {
    out = SoundBytes{};
    juce::AudioBuffer<float> buf;
    SoundFileInfo info;
    if (!loadSoundFile(uri, buf, info) || buf.getNumSamples() <= 0) return false;
    out.sampleRate = info.sampleRate;
    out.channels.resize((std::size_t) buf.getNumChannels());
    for (int c = 0; c < buf.getNumChannels(); ++c)
        out.channels[(std::size_t) c].assign(buf.getReadPointer(c),
                                             buf.getReadPointer(c) + buf.getNumSamples());
    return true;
}

inline std::string soundFileStamp(const juce::File& f, int track = 0) {
    return f.getFullPathName().toStdString() + "?"
         + std::to_string(f.getLastModificationTime().toMilliseconds()) + "+"
         + std::to_string(f.getSize()) + "#" + std::to_string(track);
}

namespace detail {

struct SharedSoundSlot {
    std::string key;
    std::weak_ptr<const juce::AudioBuffer<float>> audio;
    SoundFileInfo info;
};

inline juce::CriticalSection& sharedSoundLock() {
    static juce::CriticalSection lock;
    return lock;
}

inline std::vector<SharedSoundSlot>& sharedSoundSlots() {
    static std::vector<SharedSoundSlot> slots;
    return slots;
}

}

inline std::atomic<unsigned>& sharedSoundHits() {
    static std::atomic<unsigned> hits{0};
    return hits;
}

inline SharedSound warmSoundFile(std::string uri, int track = 0) {
    if (uri.rfind("file://", 0) == 0) uri = uri.substr(7);
    if (uri.empty()) return {};
    const auto key = soundFileStamp(juce::File(juce::String(juce::CharPointer_UTF8(uri.c_str()))),
                                    track);
    const juce::ScopedLock sl(detail::sharedSoundLock());
    for (const auto& slot : detail::sharedSoundSlots())
        if (slot.key == key)
            if (auto held = slot.audio.lock()) {
                ++sharedSoundHits();
                return {held, slot.info};
            }
    return {};
}

inline SharedSound loadSharedSoundFile(std::string uri, const SoundFileProgress& onProgress = {},
                                       double maxSeconds = 0.0,
                                       const SoundFileAbort& abandon = {}, int track = 0,
                                       const SoundFileOpened& onOpened = {}) {
    if (uri.rfind("file://", 0) == 0) uri = uri.substr(7);
    if (uri.empty()) return {};
    if (auto warm = warmSoundFile(uri, track); warm.audio != nullptr) {
        if (onProgress) onProgress(1.0f);
        return warm;
    }
    const juce::File f(juce::String(juce::CharPointer_UTF8(uri.c_str())));
    const auto key = soundFileStamp(f, track);
    auto fresh = std::make_shared<juce::AudioBuffer<float>>();
    SoundFileInfo info;
    SoundFill filled;
    SoundFileFilling filling;
    if (onOpened) {
        filled = std::make_shared<std::atomic<std::int64_t>>(0);
        filling = [&](std::int64_t at, std::int64_t) {
            const bool first = filled->load() == 0;
            filled->store(at);
            if (first) onOpened({fresh, info}, filled);
        };
    }
    if (!loadSoundFile(uri, *fresh, info, onProgress, maxSeconds, abandon, track, filling)) return {};
    if (filled != nullptr) filled->store(fresh->getNumSamples());
    const SharedAudio held = fresh;
    if (maxSeconds > 0.0) return {held, info};
    {
        const juce::ScopedLock sl(detail::sharedSoundLock());
        auto& slots = detail::sharedSoundSlots();
        slots.erase(std::remove_if(slots.begin(), slots.end(),
                                   [](const detail::SharedSoundSlot& slot) {
                                       return slot.audio.expired();
                                   }),
                    slots.end());
        slots.push_back({key, held, info});
    }
    return {held, info};
}

inline bool loadSoundFile(std::string uri, juce::AudioBuffer<float>& dest,
                          double& fileSampleRate, const SoundFileProgress& onProgress = {}) {
    SoundFileInfo info;
    if (!loadSoundFile(std::move(uri), dest, info, onProgress)) return false;
    fileSampleRate = info.sampleRate;
    return true;
}

inline juce::StringPairArray loopTempoMetadata(double tempoBpm, int numSamples, double sampleRate) {
    juce::StringPairArray meta;
    if (tempoBpm <= 0.0 || sampleRate <= 0.0) return meta;
    const int beats = juce::roundToInt((double) numSamples / sampleRate * tempoBpm / kSecondsPerMinute);
    meta.set(juce::WavAudioFormat::acidStretch, "1");
    meta.set(juce::WavAudioFormat::acidNumerator, "4");
    meta.set(juce::WavAudioFormat::acidDenominator, "4");
    meta.set(juce::WavAudioFormat::acidBeats, juce::String(juce::jmax(1, beats)));
    meta.set(juce::WavAudioFormat::acidTempo, juce::String(tempoBpm));
    return meta;
}

inline bool writeSoundFile(const std::string& path, const juce::AudioBuffer<float>& buf,
                           double sampleRate, int numSamples = -1, int bitsPerSample = 24,
                           double tempoBpm = 0.0) {
    const int n = numSamples < 0 ? buf.getNumSamples() : std::min(numSamples, buf.getNumSamples());
    if (path.empty() || n <= 0 || buf.getNumChannels() < 1 || sampleRate <= 0.0) return false;
    juce::File f(juce::String(juce::CharPointer_UTF8(path.c_str())));
    f.getParentDirectory().createDirectory();
    f.deleteFile();
    auto stream = f.createOutputStream();
    if (!stream) return false;
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> w(
        wav.createWriterFor(stream.get(), sampleRate, (unsigned int) buf.getNumChannels(),
                            bitsPerSample, loopTempoMetadata(tempoBpm, n, sampleRate), 0));
    if (!w) return false;
    stream.release();
    return w->writeFromAudioSampleBuffer(buf, 0, n);
}

}
