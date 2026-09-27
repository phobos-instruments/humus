// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include "gui/editor/LiveControls.h"
#include "hum/caps/Audio.h"
#include "hum/dsp/DspMath.h"

namespace hum::readout {

class Spectrum {
public:
    static constexpr int kOrder = 11, kFftSize = 1 << kOrder;
    static constexpr int kCols = 192, kRows = 128;
    static constexpr float kFloorDb = -72.0f, kLoHz = 30.0f, kHiHz = 18000.0f;

    Spectrum() : cols_(kCols, kFloorDb), peaks_(kCols, kFloorDb) {
        for (int i = 0; i < kFftSize; ++i)
            window_[i] = 0.5f * (1.0f - std::cos(kTwoPiF * (float) i / (float) (kFftSize - 1)));
    }

    bool poll(ModelHost& host, const std::string& organism, float* frame, double& rate) {
        auto* src = live::source<ScopeSource>(host, organism);
        if (src == nullptr) return false;
        const unsigned stamp = src->scopeStamp();
        if (stamp == lastStamp_) return false;
        lastStamp_ = stamp;
        std::fill(frame, frame + kFftSize, 0.0f);
        src->scopeRead(frame, kFftSize);
        rate = src->scopeRate();
        return true;
    }

    template <class MagnitudeTransform>
    void analyze(const float* samples, double sr, MagnitudeTransform&& transform) {
        float buf[2 * kFftSize] = {};
        for (int i = 0; i < kFftSize; ++i) buf[i] = samples[i] * window_[i];
        transform(buf);
        const double hzPerBin = sr / (double) kFftSize;
        head_ = (head_ + kRows - 1) % kRows;
        for (int c = 0; c < kCols; ++c) {
            const double f0 = kLoHz * std::pow((double) kHiHz / kLoHz, (double) c / (double) kCols);
            const double f1 = kLoHz * std::pow((double) kHiHz / kLoHz, (double) (c + 1) / (double) kCols);
            int b0 = std::max(1, (int) (f0 / hzPerBin));
            int b1 = std::max(b0 + 1, (int) std::ceil(f1 / hzPerBin));
            b1 = std::min(b1, kFftSize / 2);
            float mag = 0.0f;
            for (int b = b0; b < b1; ++b) mag = std::max(mag, buf[b]);
            const float db = std::clamp(20.0f * std::log10(std::max(1.0e-9f, mag * 2.0f / (float) kFftSize)),
                                        kFloorDb, 0.0f);
            auto& col = cols_[(size_t) c];
            col = db > col ? db : col - 2.4f;
            if (col < kFloorDb) col = kFloorDb;
            auto& pk = peaks_[(size_t) c];
            pk = std::max(db, pk - 0.35f);
            if (pk < kFloorDb) pk = kFloorDb;
            heat_[c] = (db - kFloorDb) / -kFloorDb;
        }
    }

    static int previewFrames() { return kRows - 16; }

    static double xOfHz(double hz) { return std::log(hz / kLoHz) / std::log((double) kHiHz / kLoHz); }
    static float heightOfDb(float db) { return std::clamp((db - kFloorDb) / -kFloorDb, 0.0f, 1.0f); }

    int head() const { return head_; }
    float column(int c) const { return cols_[(size_t) c]; }
    float peak(int c) const { return peaks_[(size_t) c]; }
    float heat(int c) const { return heat_[c]; }

private:
    float window_[kFftSize];
    std::vector<float> cols_, peaks_;
    float heat_[kCols] = {};
    int head_ = 0;
    unsigned lastStamp_ = ~0u;
};

}
