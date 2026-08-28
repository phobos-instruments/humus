#include "io/WavWriter.h"

#include <memory>

#include <juce_audio_formats/juce_audio_formats.h>

namespace hum {

bool writeWav(const std::string& path,
              const std::vector<std::vector<float>>& channels,
              double sampleRate, int bitsPerSample) {
    if (channels.empty()) return false;
    const int numCh = (int) channels.size();
    const int64_t numSamples = (int64_t) channels[0].size();

    juce::File f(juce::String(juce::CharPointer_UTF8(path.c_str())));
    f.deleteFile();
    std::unique_ptr<juce::FileOutputStream> stream(f.createOutputStream());
    if (!stream) return false;

    juce::WavAudioFormat fmt;
    std::unique_ptr<juce::AudioFormatWriter> writer(
        fmt.createWriterFor(stream.get(), sampleRate, (unsigned) numCh,
                            bitsPerSample, {}, 0));
    if (!writer) return false;
    stream.release();

    std::vector<const float*> ptrs(numCh);
    for (int c = 0; c < numCh; ++c) ptrs[c] = channels[c].data();
    bool ok = writer->writeFromFloatArrays(ptrs.data(), numCh, (int) numSamples);
    return ok;
}

}
