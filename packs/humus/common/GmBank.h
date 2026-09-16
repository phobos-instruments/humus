// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <string>

#include <juce_core/juce_core.h>

#include "common/FmChip.h"

namespace hum {

class GmBank {
public:
    static constexpr int kPrograms = 128;

    struct Entry {
        std::string name;
        FmChip::Patch patch;
        int noteOffset = 0;
        int percussionKey = 0;
    };

    bool load(const juce::File& file);
    bool ready() const { return ready_; }
    bool hasPercussion() const { return hasPercussion_; }
    const std::string& name() const { return name_; }

    const Entry& melodic(int program) const;
    const Entry& percussion(int note) const;

private:
    static int wrap(int i) { return i < 0 ? 0 : i >= kPrograms ? kPrograms - 1 : i; }

    std::array<Entry, kPrograms> melodic_{};
    std::array<Entry, kPrograms> percussion_{};
    bool ready_ = false;
    bool hasPercussion_ = false;
    std::string name_;
};

}
