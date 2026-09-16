// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <memory>
#include <string>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>

#include "hum/dsp/DspMath.h"

namespace hum {

struct SoundFileInfo {
    double sampleRate = kDefaultSampleRate;
    int rootKey = -1;
    int loopStart = 0, loopEnd = 0;
};

inline bool loadSoundFile(std::string uri, juce::AudioBuffer<float>& dest,
                          SoundFileInfo& info) {
    dest.setSize(0, 0);
    info = SoundFileInfo{};
    if (uri.empty()) return false;
    if (uri.rfind("file://", 0) == 0) uri = uri.substr(7);
    juce::File f(juce::String(juce::CharPointer_UTF8(uri.c_str())));
    if (!f.existsAsFile()) return false;

    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(f));
    if (!reader || reader->lengthInSamples <= 0) return false;

    dest.setSize((int) reader->numChannels, (int) reader->lengthInSamples);
    reader->read(&dest, 0, (int) reader->lengthInSamples, 0, true, true);
    info.sampleRate = reader->sampleRate > 0.0 ? reader->sampleRate : kDefaultSampleRate;

    const auto& meta = reader->metadataValues;
    if (meta.containsKey("MidiUnityNote"))
        info.rootKey = juce::jlimit(0, kMidiMax, meta["MidiUnityNote"].getIntValue());
    if (meta.getValue("NumSampleLoops", "0").getIntValue() > 0) {
        info.loopStart = meta["Loop0Start"].getIntValue();
        info.loopEnd = meta["Loop0End"].getIntValue();
    }
    const auto len = (int) reader->lengthInSamples;
    info.loopStart = juce::jlimit(0, len - 1, info.loopStart);
    info.loopEnd = juce::jlimit(0, len - 1, info.loopEnd);
    return true;
}

inline bool loadSoundFile(std::string uri, juce::AudioBuffer<float>& dest,
                          double& fileSampleRate) {
    SoundFileInfo info;
    if (!loadSoundFile(std::move(uri), dest, info)) return false;
    fileSampleRate = info.sampleRate;
    return true;
}

inline bool writeSoundFile(const std::string& path, const juce::AudioBuffer<float>& buf,
                           double sampleRate, int numSamples = -1, int bitsPerSample = 24) {
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
                            bitsPerSample, {}, 0));
    if (!w) return false;
    stream.release();
    return w->writeFromAudioSampleBuffer(buf, 0, n);
}

}
