// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <cstddef>
#include <string_view>
#include <vector>

#include "hum/dsp/DspMath.h"

namespace hum {

enum class MidiMessageType { ControlChange, Note, PitchBend, ChannelPressure };
enum class ControlType { Fader, Button, Encoder };
enum class ButtonMode { Hold, Toggle, Set, StepUp, StepDown, Reset };
enum class FaderMode { Direct, PickUp };
enum class EncoderFormat { TwosComplement, Offset64, SignBit };
enum class ControlFamily { Midi, Osc, Mod };

template <typename E>
struct ModeRow {
    E value;
    const char* word;
    const char* label;
};

template <typename E>
struct ModeTable;

template <>
struct ModeTable<MidiMessageType> {
    static constexpr std::array<ModeRow<MidiMessageType>, 4> rows{{
        {MidiMessageType::ControlChange, "7-bit-control-change", "CC"},
        {MidiMessageType::Note, "note", "Note"},
        {MidiMessageType::PitchBend, "pitch-bend", "Pitch bend"},
        {MidiMessageType::ChannelPressure, "channel-pressure", "Pressure"},
    }};
};

template <>
struct ModeTable<ControlType> {
    static constexpr std::array<ModeRow<ControlType>, 3> rows{{
        {ControlType::Fader, "fader", "Knob / Fader"},
        {ControlType::Button, "button", "Button / Pad"},
        {ControlType::Encoder, "encoder", "Encoder"},
    }};
};

template <>
struct ModeTable<ButtonMode> {
    static constexpr std::array<ModeRow<ButtonMode>, 6> rows{{
        {ButtonMode::Hold, "hold", "Hold"},
        {ButtonMode::Toggle, "toggle", "Toggle"},
        {ButtonMode::Set, "set", "Set value"},
        {ButtonMode::StepUp, "step-up", "Step up"},
        {ButtonMode::StepDown, "step-down", "Step down"},
        {ButtonMode::Reset, "reset", "Reset"},
    }};
};

template <>
struct ModeTable<FaderMode> {
    static constexpr std::array<ModeRow<FaderMode>, 2> rows{{
        {FaderMode::Direct, "direct", "Direct"},
        {FaderMode::PickUp, "pick-up", "Pick up"},
    }};
};

template <>
struct ModeTable<EncoderFormat> {
    static constexpr std::array<ModeRow<EncoderFormat>, 3> rows{{
        {EncoderFormat::TwosComplement, "twos-complement", "7F / 01"},
        {EncoderFormat::Offset64, "offset-64", "3F / 41"},
        {EncoderFormat::SignBit, "sign-bit", "41 / 01"},
    }};
};

template <typename E>
constexpr const ModeRow<E>& modeRow(E v) {
    for (const auto& r : ModeTable<E>::rows)
        if (r.value == v) return r;
    return ModeTable<E>::rows[0];
}

template <typename E>
constexpr const char* modeWord(E v) { return modeRow(v).word; }

template <typename E>
constexpr const char* modeLabel(E v) { return modeRow(v).label; }

template <typename E>
constexpr E parseMode(std::string_view word, E fallback) {
    for (const auto& r : ModeTable<E>::rows)
        if (word == r.word) return r.value;
    return fallback;
}

inline bool buttonModeIsAbsolute(ButtonMode m) {
    return m == ButtonMode::Hold || m == ButtonMode::Toggle || m == ButtonMode::Set;
}

inline bool buttonModeUsesStep(ButtonMode m) {
    return m == ButtonMode::StepUp || m == ButtonMode::StepDown;
}

inline std::vector<ControlType> controlTypesFor(ControlFamily f) {
    if (f == ControlFamily::Midi) return {ControlType::Fader, ControlType::Button, ControlType::Encoder};
    return {ControlType::Fader, ControlType::Button};
}

inline std::vector<ButtonMode> buttonModesFor(ControlFamily f) {
    if (f == ControlFamily::Mod) return {ButtonMode::Hold, ButtonMode::Toggle};
    std::vector<ButtonMode> out;
    for (const auto& r : ModeTable<ButtonMode>::rows) out.push_back(r.value);
    return out;
}

inline std::vector<FaderMode> faderModesFor(ControlFamily f) {
    if (f == ControlFamily::Midi) return {FaderMode::Direct, FaderMode::PickUp};
    return {FaderMode::Direct};
}

inline constexpr double kDefaultControlStep = 0.05;

inline int decodeEncoder(EncoderFormat f, int value7) {
    if (value7 <= 0 || value7 > kMidiMax) return 0;
    switch (f) {
        case EncoderFormat::TwosComplement: return value7 < 64 ? value7 : value7 - 128;
        case EncoderFormat::Offset64:       return value7 - 64;
        case EncoderFormat::SignBit:        return value7 < 64 ? value7 : -(value7 - 64);
    }
    return 0;
}

inline constexpr std::size_t kEncoderFormats = ModeTable<EncoderFormat>::rows.size();

}
