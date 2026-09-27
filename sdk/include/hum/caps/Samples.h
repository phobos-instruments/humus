// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace hum {

class SoundMapSource {
public:
    struct MapPoint { float x = 0.0f, y = 0.0f; int file = 0; };
    static constexpr int kLiveFile = 4;
    static constexpr int kFreshLiveFile = 5;
    virtual ~SoundMapSource() = default;
    virtual unsigned mapGeneration() const = 0;
    virtual const std::vector<MapPoint>& mapPoints() const = 0;
};

class GrainFlashSource {
public:
    struct Flash { float x = 0.0f, y = 0.0f, seconds = 0.0f; int file = 0; std::uint32_t serial = 0; };
    virtual ~GrainFlashSource() = default;
    virtual int recentGrains(Flash* out, int capacity) const = 0;
};

class SliceSource {
public:
    virtual ~SliceSource() = default;
    virtual unsigned sliceGeneration() const = 0;
    virtual const std::vector<float>& slicePeaks() const = 0;
    virtual std::vector<float> sliceStarts() const = 0;
    virtual int playingSlice() const = 0;
};

class SliceWorkbench {
public:
    virtual ~SliceWorkbench() = default;
    virtual std::string pinParam() const = 0;
    virtual std::string pinValue(int index) const = 0;
    virtual std::string pinAlternative(int index) const = 0;
    virtual std::string pinNudged(const std::string& from, int sourceStep,
                                  int fragmentStep) const = 0;
    virtual std::string startParam() const = 0;
    virtual int sliceOrigin(int index) const = 0;
    virtual int originCount() const = 0;
    virtual std::string originName(int origin) const = 0;
    virtual void auditionSlice(int index) = 0;
};

}
