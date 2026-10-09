// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "hum/Pattern.h"

namespace hum {

inline double warpRate(int mode, double sourceBpm, double projectBpm) {
    if (mode == (int) PatternChannel::Warp::Off || sourceBpm <= 0.0 || projectBpm <= 0.0) return 1.0;
    return projectBpm / sourceBpm;
}

inline double warpRate(const PatternChannel& clip, double projectBpm) {
    return warpRate(clip.warpMode, clip.sourceBpm, projectBpm);
}

}
