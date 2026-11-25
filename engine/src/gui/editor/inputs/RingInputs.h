// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>

#include "gui/host/BrickHost.h"
#include "hum/Euclid.h"
#include "hum/RingMutes.h"

namespace hum::input {

class RingMuteModel {
public:
    RingMuteModel(BrickHost& host, std::string organism, std::string hitsPrefix, std::string turnPrefix,
                  std::string mutePrefix)
        : host_(host), organism_(std::move(organism)), hitsPrefix_(std::move(hitsPrefix)),
          turnPrefix_(std::move(turnPrefix)), mutePrefix_(std::move(mutePrefix)) {}

    bool editable() const { return !hitsPrefix_.empty() && !turnPrefix_.empty() && !mutePrefix_.empty(); }

    std::uint32_t mask(int ring) const { return ringmute::maskFrom(value(mutePrefix_, ring)); }

    std::uint64_t mutedHits(int ring, int steps) const {
        if (!editable()) return 0;
        const int hits = whole(hitsPrefix_, ring), turn = whole(turnPrefix_, ring);
        const auto muted = mask(ring);
        std::uint64_t out = 0;
        for (int s = 0; s < steps && s < 64; ++s)
            if (euclid::hit(s, steps, hits, turn) && (muted & ringmute::bitAt(s, steps, turn)) != 0)
                out |= std::uint64_t{1} << s;
        return out;
    }

    bool toggle(int ring, int step, int steps) {
        if (!editable()) return false;
        const int hits = whole(hitsPrefix_, ring), turn = whole(turnPrefix_, ring);
        if (!euclid::hit(step, steps, hits, turn)) return false;
        setMask(ring, mask(ring) ^ ringmute::bitAt(step, steps, turn));
        return true;
    }

private:
    std::string param(const std::string& prefix, int ring) const { return prefix + std::to_string(ring + 1); }
    double value(const std::string& prefix, int ring) const {
        return host_.liveParamValue(organism_, param(prefix, ring));
    }
    int whole(const std::string& prefix, int ring) const { return (int) std::lround(value(prefix, ring)); }
    void setMask(int ring, std::uint32_t m) { host_.editParam(organism_, param(mutePrefix_, ring), ringmute::valueFrom(m)); }

    BrickHost& host_;
    std::string organism_, hitsPrefix_, turnPrefix_, mutePrefix_;
};

}
