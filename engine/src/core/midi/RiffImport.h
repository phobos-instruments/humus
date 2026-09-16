// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

#include "hum/PatternMatrix.h"

namespace hum::riff {

inline constexpr int kBlockBytes = 110;
inline constexpr int kMaxSteps = 16;
inline constexpr int kMaxWholePattern = 32;
inline constexpr int kPitchOffset = 24;
inline constexpr std::uint8_t kSysexHeader[] = {0xF0, 0x00, 0x20, 0x32, 0x00, 0x01, 0x0A, 0x78};
inline constexpr std::uint8_t kSeqMagic[] = {0x23, 0x98, 0x54, 0x76};

enum class Format { Unknown, Sysex, Seq, Midi };

struct Imported {
    std::vector<BasslineStep> steps;
    bool triplet = false;
};

Format sniff(const std::uint8_t* data, std::size_t size);

bool decodeBlock(const std::uint8_t* block, Imported& out);

std::vector<Imported> importBytes(const std::uint8_t* data, std::size_t size);

}
