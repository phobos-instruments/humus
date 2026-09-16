// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "hum/dsp/LiveWavWriter.h"

namespace hum {

bool LiveWavWriter::start(const std::string& path, int numChannels, double sampleRate, bool append) {
    stop();
    if (numChannels < 1 || sampleRate <= 0.0) return false;

    juce::File f(juce::String(juce::CharPointer_UTF8(path.c_str())));

    juce::AudioBuffer<float> existing;
    if (append && f.existsAsFile()) {
        juce::AudioFormatManager fm; fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> rd(fm.createReaderFor(f));
        if (rd && (int) rd->numChannels == numChannels && rd->lengthInSamples > 0) {
            existing.setSize(numChannels, (int) rd->lengthInSamples);
            rd->read(&existing, 0, (int) rd->lengthInSamples, 0, true, true);
        }
    }

    f.deleteFile();
    auto stream = f.createOutputStream();
    if (!stream) return false;

    juce::WavAudioFormat wav;
    auto* raw = wav.createWriterFor(stream.get(), sampleRate, (unsigned int) numChannels, 16, {}, 0);
    if (!raw) return false;
    stream.release();

    if (existing.getNumSamples() > 0)
        raw->writeFromAudioSampleBuffer(existing, 0, existing.getNumSamples());

    thread_.startThread();
    writer_ = std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(raw, thread_, kFifoSamples);
    channels_ = numChannels;
    return true;
}

bool LiveWavWriter::write(const float* const* data, int numSamples) {
    if (!writer_) return false;
    return writer_->write(data, numSamples);
}

void LiveWavWriter::stop() {
    writer_.reset();
    thread_.stopThread(2000);
    channels_ = 0;
}

}
