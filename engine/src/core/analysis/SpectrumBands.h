// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <cmath>
#include <cstddef>

namespace hum {
namespace spectrum {

struct Band {
    const char* name;
    double lo, hi;
};

inline const std::array<Band, 8>& bands() {
    static const std::array<Band, 8> b{{{"sub", 20.0, 60.0},
                                        {"bass", 60.0, 150.0},
                                        {"low_mid", 150.0, 400.0},
                                        {"mid", 400.0, 1000.0},
                                        {"high_mid", 1000.0, 2500.0},
                                        {"presence", 2500.0, 6000.0},
                                        {"brilliance", 6000.0, 12000.0},
                                        {"air", 12000.0, 20000.0}}};
    return b;
}

constexpr double kSilenceDb = -80.0;

inline std::array<double, 8> bandDb(const float* mags, int numBins, double sampleRate) {
    std::array<double, 8> sum{};
    std::array<int, 8> count{};
    const double hzPerBin = sampleRate / (2.0 * (double) numBins);
    for (int i = 1; i < numBins; ++i) {
        const double hz = i * hzPerBin;
        for (size_t b = 0; b < bands().size(); ++b) {
            if (hz >= bands()[b].lo && hz < bands()[b].hi) {
                sum[b] += (double) mags[i] * (double) mags[i];
                ++count[b];
                break;
            }
        }
    }
    std::array<double, 8> db{};
    for (size_t b = 0; b < db.size(); ++b) {
        const double rms = count[b] > 0 ? std::sqrt(sum[b] / count[b]) : 0.0;
        db[b] = rms > 1.0e-4 ? std::max(kSilenceDb, 20.0 * std::log10(rms)) : kSilenceDb;
    }
    return db;
}

}
}
