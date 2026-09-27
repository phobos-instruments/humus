// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "core/midi/MidiSource.h"
#include "core/net/ControlShape.h"
#include "gui/common/Localisation.h"

namespace hum {

inline juce::String modeText(ControlType t) {
    switch (t) {
        case ControlType::Fader:   return tr("control-mode.fader", "Knob / Fader");
        case ControlType::Button:  return tr("control-mode.button", "Button / Pad");
        case ControlType::Encoder: return tr("control-mode.encoder", "Encoder");
    }
    return {};
}

inline juce::String modeText(ButtonMode m) {
    switch (m) {
        case ButtonMode::Hold:     return tr("control-mode.hold", "Hold");
        case ButtonMode::Toggle:   return tr("control-mode.toggle", "Toggle");
        case ButtonMode::Set:      return tr("control-mode.set", "Set value");
        case ButtonMode::StepUp:   return tr("control-mode.step-up", "Step up");
        case ButtonMode::StepDown: return tr("control-mode.step-down", "Step down");
        case ButtonMode::Reset:    return tr("control-mode.reset", "Reset");
    }
    return {};
}

inline juce::String modeText(FaderMode m) {
    switch (m) {
        case FaderMode::Direct: return tr("control-mode.direct", "Direct");
        case FaderMode::PickUp: return tr("control-mode.pick-up", "Pick up");
    }
    return {};
}

inline juce::String modeText(EncoderFormat f) { return juce::String(modeLabel(f)); }

inline juce::String modeText(MidiMessageType t) {
    switch (t) {
        case MidiMessageType::ControlChange:   return tr("control-mode.cc", "CC");
        case MidiMessageType::Note:            return tr("control-mode.note", "Note");
        case MidiMessageType::PitchBend:       return tr("control-mode.pitch-bend", "Bend");
        case MidiMessageType::ChannelPressure: return tr("control-mode.pressure", "Pressure");
    }
    return {};
}

inline juce::String behaviourText(const ControlShape& s) {
    switch (s.type) {
        case ControlType::Button:  return modeText(s.button);
        case ControlType::Fader:   return modeText(s.fader);
        case ControlType::Encoder: return modeText(ControlType::Encoder);
    }
    return {};
}

inline juce::String behaviourHint(const ControlShape& s) {
    if (s.isEncoder())
        return tr("control-mode.hint-encoder",
                  "An endless knob. Each detent nudges the parameter by Step. Pick the format "
                  "your controller sends; Learn guesses it for you.");
    if (s.isFader())
        return s.fader == FaderMode::PickUp
                   ? tr("control-mode.hint-pick-up",
                        "Nothing moves until the knob passes where the parameter already is, "
                        "so a preset or an on-screen change never jumps. Right-click a point to remove it.")
                   : tr("control-mode.hint-direct",
                        "The parameter jumps straight to where the knob is, even if they were far "
                        "apart. Click the curve to add a point, right-click a point to remove it.");
    switch (s.button) {
        case ButtonMode::Hold:
            return tr("control-mode.hint-hold", "On while held, off when released.");
        case ButtonMode::Toggle:
            return tr("control-mode.hint-toggle", "Each press flips it on or off.");
        case ButtonMode::Set:
            return tr("control-mode.hint-set", "Each press sets the parameter to the To value.");
        case ButtonMode::StepUp:
            return tr("control-mode.hint-step-up",
                      "Each press moves the parameter up by Step. Lists and whole numbers move "
                      "one item.");
        case ButtonMode::StepDown:
            return tr("control-mode.hint-step-down",
                      "Each press moves the parameter down by Step. Lists and whole numbers "
                      "move one item.");
        case ButtonMode::Reset:
            return tr("control-mode.hint-reset", "Each press returns the parameter to its default.");
    }
    return {};
}

inline juce::String heldSourceText(int source) {
    if (isNoteSource(source)) return juce::String(midiNoteName(noteOfSource(source)));
    return juce::String(midiSourceLabel(source));
}

inline juce::String channelText(int channel) {
    return midiChannelSlot(channel) == kAnyMidiChannel
               ? tr("control-mode.any-channel", "All ch")
               : tr("control-mode.channel", "ch") + " " + juce::String(channel);
}

}
