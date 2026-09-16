// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <string>

#include "gui/host/BrickHost.h"

namespace hum {

class VideoHost : public virtual BrickHost {
public:
    ~VideoHost() override = default;

    virtual std::string videoSourceInto(const std::string& dst, int dstPort) const = 0;
    virtual std::string videoSourceInto(const std::string& dst, int dstPort, int& srcOutlet) const = 0;
    virtual double sampleRate() const = 0;
    virtual int blockSize() const = 0;
    virtual void stop() = 0;
    virtual void primeOffline(int blocks) = 0;
    virtual void advanceModulation(double dt) = 0;
    virtual void holdAudio(bool held) = 0;
};

}
