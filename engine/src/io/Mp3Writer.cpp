// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "io/Mp3Writer.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <vector>

#include <juce_core/juce_core.h>

#include <lame.h>

namespace hum {

namespace {

constexpr int kBlock = 8192;
constexpr double kBitsPerByte = 8.0;

struct Encoder {
    lame_global_flags* flags = nullptr;

    ~Encoder() {
        if (flags != nullptr) lame_close(flags);
    }
};

int roomFor(int frames) { return frames + frames / 4 + 7200; }

}

double mp3BytesPerSecond(int rate) {
    const int step = juce::jlimit(0, kMp3RateSteps - 1, rate);
    return kMp3KbitPerSecond[step] * 1.0e3 / kBitsPerByte;
}

bool writeMp3(const std::string& path,
              const std::vector<std::vector<float>>& channels,
              double sampleRate, int rate) {
    if (channels.empty() || channels[0].empty() || sampleRate <= 0.0) return false;
    const int wide = std::min((int) channels.size(), 2);
    const auto frames = (int) channels[0].size();

    Encoder lame;
    lame.flags = lame_init();
    if (lame.flags == nullptr) return false;
    lame_set_num_channels(lame.flags, wide);
    lame_set_in_samplerate(lame.flags, (int) juce::roundToInt(sampleRate));
    lame_set_mode(lame.flags, wide == 1 ? MONO : JOINT_STEREO);
    lame_set_VBR(lame.flags, vbr_default);
    lame_set_VBR_q(lame.flags, kMp3VbrQuality[juce::jlimit(0, kMp3RateSteps - 1, rate)]);
    lame_set_bWriteVbrTag(lame.flags, 1);
    if (lame_init_params(lame.flags) < 0) return false;

    std::vector<unsigned char> out;
    std::vector<unsigned char> block((size_t) roomFor(kBlock));
    const float* left = channels[0].data();
    const float* right = channels[(size_t) (wide > 1 ? 1 : 0)].data();
    for (int at = 0; at < frames; at += kBlock) {
        const int take = std::min(kBlock, frames - at);
        const int wrote = lame_encode_buffer_ieee_float(lame.flags, left + at, right + at, take,
                                                        block.data(), (int) block.size());
        if (wrote < 0) return false;
        out.insert(out.end(), block.begin(), block.begin() + wrote);
    }
    const int tail = lame_encode_flush(lame.flags, block.data(), (int) block.size());
    if (tail < 0) return false;
    out.insert(out.end(), block.begin(), block.begin() + tail);

    std::vector<unsigned char> tag((size_t) roomFor(0));
    const auto tagged = lame_get_lametag_frame(lame.flags, tag.data(), tag.size());
    if (tagged > 0 && tagged <= out.size()) std::memcpy(out.data(), tag.data(), tagged);

    const juce::File file(juce::String(juce::CharPointer_UTF8(path.c_str())));
    file.deleteFile();
    return file.replaceWithData(out.data(), out.size());
}

}
