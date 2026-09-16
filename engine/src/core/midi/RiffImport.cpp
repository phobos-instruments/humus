// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/midi/RiffImport.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace hum::riff {

std::vector<Imported> importMidi(const std::uint8_t* data, std::size_t size);

namespace {

constexpr int kNotesAt = 0;
constexpr int kAccentsAt = 32;
constexpr int kSlidesAt = 64;
constexpr int kTripletAt = 96;
constexpr int kLengthAt = 98;
constexpr int kGatesAt = 102;
constexpr int kRestsAt = 106;
constexpr int kSysexBlockAt = 12;
constexpr std::uint8_t kHighCFlag = 0x08;
static_assert(kRestsAt + 4 == kBlockBytes, "the rest mask closes the block");

bool starts(const std::uint8_t* data, std::size_t size, const std::uint8_t* with, std::size_t n) {
    return size >= n && std::memcmp(data, with, n) == 0;
}

int pair(const std::uint8_t* at) { return ((at[0] & 0x0F) << 4) | (at[1] & 0x0F); }

unsigned mask(const std::uint8_t* at) {
    return ((unsigned) (at[0] & 0x0F) << 4) | (unsigned) (at[1] & 0x0F)
           | ((unsigned) (at[2] & 0x0F) << 12) | ((unsigned) (at[3] & 0x0F) << 8);
}

bool flagPair(const std::uint8_t* at) { return at[0] == 0 && at[1] <= 1; }

bool plausible(const std::uint8_t* block) {
    for (int i = 0; i < kMaxSteps; ++i) {
        const std::uint8_t* note = block + kNotesAt + i * 2;
        if ((note[0] & ~kHighCFlag) > 0x07 || note[1] > 0x0F) return false;
        if (!flagPair(block + kAccentsAt + i * 2) || !flagPair(block + kSlidesAt + i * 2))
            return false;
    }
    const int length = pair(block + kLengthAt);
    return flagPair(block + kTripletAt) && length >= 1 && length <= kMaxSteps;
}

}

Format sniff(const std::uint8_t* data, std::size_t size) {
    if (size > 0 && data[0] == kSysexHeader[0]) return Format::Sysex;
    if (starts(data, size, kSeqMagic, sizeof(kSeqMagic))) return Format::Seq;
    static constexpr std::uint8_t midi[] = {'M', 'T', 'h', 'd'};
    if (starts(data, size, midi, sizeof(midi))) return Format::Midi;
    return Format::Unknown;
}

bool decodeBlock(const std::uint8_t* block, Imported& out) {
    if (block == nullptr || !plausible(block)) return false;
    const int length = pair(block + kLengthAt);
    const unsigned fresh = mask(block + kGatesAt);
    const unsigned rests = mask(block + kRestsAt);
    out.triplet = block[kTripletAt + 1] != 0;
    out.steps.assign((size_t) length, BasslineStep{});

    int pitch = 0;
    int sounding = -1;
    for (int t = 0; t < length; ++t) {
        BasslineStep& step = out.steps[(size_t) t];
        if (rests & (1u << t)) { sounding = -1; continue; }
        const bool tie = !(fresh & (1u << t)) && sounding >= 0;
        if (tie) {
            BasslineStep& held = out.steps[(size_t) sounding];
            step = held;
            held.slide = true;
            sounding = t;
            continue;
        }
        const std::uint8_t* note = block + kNotesAt + pitch * 2;
        step.gate = true;
        step.note = kPitchOffset + (pair(note) & 0x7F);
        step.accent = block[kAccentsAt + pitch * 2 + 1] != 0;
        step.slide = block[kSlidesAt + pitch * 2 + 1] != 0;
        pitch = std::min(pitch + 1, kMaxSteps - 1);
        sounding = t;
    }
    return true;
}

std::vector<Imported> importBytes(const std::uint8_t* data, std::size_t size) {
    std::vector<Imported> out;
    if (data == nullptr || size == 0) return out;
    switch (sniff(data, size)) {
        case Format::Sysex:
            for (std::size_t at = 0; at + kSysexBlockAt + kBlockBytes < size;) {
                if (!starts(data + at, size - at, kSysexHeader, sizeof(kSysexHeader))) {
                    ++at;
                    continue;
                }
                Imported riff;
                if (decodeBlock(data + at + kSysexBlockAt, riff)) out.push_back(std::move(riff));
                at += kSysexBlockAt + kBlockBytes;
            }
            break;
        case Format::Seq:
            if (size >= sizeof(kSeqMagic) + kBlockBytes) {
                Imported riff;
                if (decodeBlock(data + size - kBlockBytes, riff)) out.push_back(std::move(riff));
            }
            break;
        case Format::Midi:
            out = importMidi(data, size);
            break;
        case Format::Unknown:
            break;
    }
    return out;
}

}
