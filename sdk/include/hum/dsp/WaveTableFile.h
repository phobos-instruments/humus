// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

#include "hum/dsp/WaveTable.h"

namespace hum {

inline constexpr int kWaveFileFrameLen = 2048;
inline constexpr int kWaveFileMaxFrames = 256;

inline int waveTableFrameLenFromRiff(const std::uint8_t* bytes, std::size_t size) {
    if (bytes == nullptr || size < 12 || std::memcmp(bytes, "RIFF", 4) != 0 || std::memcmp(bytes + 8, "WAVE", 4) != 0)
        return 0;
    std::size_t at = 12;
    while (at + 8 <= size) {
        const std::uint32_t len = (std::uint32_t) bytes[at + 4] | ((std::uint32_t) bytes[at + 5] << 8)
                                | ((std::uint32_t) bytes[at + 6] << 16) | ((std::uint32_t) bytes[at + 7] << 24);
        const std::size_t body = at + 8;
        if (std::memcmp(bytes + at, "clm ", 4) == 0 && body + 3 <= size) {
            const std::size_t end = std::min(size, body + (std::size_t) len);
            std::string text((const char*) bytes + body, end - body);
            const auto tag = text.find("<!>");
            if (tag == std::string::npos) return 0;
            int frameLen = 0;
            for (std::size_t i = tag + 3; i < text.size() && text[i] >= '0' && text[i] <= '9'; ++i)
                frameLen = frameLen * 10 + (text[i] - '0');
            return frameLen >= 64 && frameLen <= 65536 ? frameLen : 0;
        }
        at = body + len + (len & 1u);
    }
    return 0;
}

inline constexpr int kWaveFileShortFrameLen = 256;
inline constexpr float kWaveFileSameFrameSimilarity = 0.9f;

inline float waveTableLagSimilarity(const float* x, int n, int lag) {
    const int m = n - lag;
    if (x == nullptr || m <= 0) return 0.0f;
    double dot = 0.0, head = 0.0, tail = 0.0;
    for (int i = 0; i < m; ++i) {
        dot += (double) x[i] * x[i + lag];
        head += (double) x[i] * x[i];
        tail += (double) x[i + lag] * x[i + lag];
    }
    return head > 0.0 && tail > 0.0 ? (float) (dot / std::sqrt(head * tail)) : 0.0f;
}

inline bool waveTableIsOneCycle(int samples) {
    return samples >= 64 && samples <= 2 * kWaveFileFrameLen && (samples & (samples - 1)) == 0;
}

inline int waveTableFrameLenGuess(const float* x, int samples) {
    if (waveTableIsOneCycle(samples)) return samples;
    if (samples % kWaveFileFrameLen != 0 || samples / kWaveFileFrameLen > kWaveFileMaxFrames) return 0;
    const float shortLag = waveTableLagSimilarity(x, samples, kWaveFileShortFrameLen);
    if (shortLag > kWaveFileSameFrameSimilarity && shortLag > waveTableLagSimilarity(x, samples, kWaveFileFrameLen))
        return kWaveFileShortFrameLen;
    return kWaveFileFrameLen;
}

inline int waveTableFramesFromTable(const float* x, int n, int frameLen, float* out, int len, int maxFrames) {
    if (x == nullptr || frameLen <= 0 || n < frameLen || maxFrames < 1) return 0;
    const int have = n / frameLen;
    const int count = std::min(have, maxFrames);
    for (int f = 0; f < count; ++f) {
        const int pick = count == 1 ? 0 : (int) ((long long) f * (have - 1) / (count - 1));
        waveTableResample(x + (std::size_t) pick * frameLen, frameLen, out + (std::size_t) f * len, len);
    }
    return count;
}

}
