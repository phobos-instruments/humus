#pragma once
#include <functional>
#include <map>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/AppSettings.h"
#include "gui/MidiKeyboardPanel.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class QwertyPiano {
public:
    static QwertyPiano& instance() {
        static QwertyPiano p;
        return p;
    }

    bool active() const {
        return enabled_ && (MidiKeyboardStrip::activeStrip() != nullptr || emitDirect != nullptr);
    }

    std::function<void(const juce::MidiMessage&)> emitDirect;

    std::function<bool(char)> keyIsDown = [](char c) {
        return juce::KeyPress::isKeyCurrentlyDown((int) c);
    };

    int baseNote() const { return baseNote_; }

    bool enabled() const { return enabled_; }
    void setEnabled(bool on) {
        if (enabled_ == on) return;
        if (!on) releaseAll();
        enabled_ = on;
        AppSettings::instance().set("qwertyPiano.enabled", on ? 1 : 0);
    }

    bool handleKeyPress(const juce::KeyPress& k) {
        if (!active() || typingInTextEditor()) return false;
        if (k.getModifiers().isAnyModifierKeyDown()) return false;
        const auto c = (char) juce::CharacterFunctions::toLowerCase(k.getTextCharacter());
        if (c == 'z' || c == 'x') {
            silenceHeld();
            baseNote_ = juce::jlimit(kMinBaseNote, kMaxBaseNote,
                                     baseNote_ + (c == 'x' ? 12 : -12));
            if (auto* strip = MidiKeyboardStrip::activeStrip())
                strip->ensureKeyRangeVisible(baseNote_, baseNote_ + (int) kKeys.size() - 1);
            return true;
        }
        return kKeys.find(c) != std::string::npos;
    }

    bool handleKeyState() {
        if (!active()) { held_.clear(); return false; }
        if (typingInTextEditor()) { releaseAll(); return false; }
        const bool shortcutHeld = juce::ModifierKeys::currentModifiers.testFlags(
            juce::ModifierKeys::ctrlModifier | juce::ModifierKeys::altModifier
            | juce::ModifierKeys::commandModifier);
        auto* strip = MidiKeyboardStrip::activeStrip();
        bool changed = false;
        for (size_t i = 0; i < kKeys.size(); ++i) {
            const char c = kKeys[i];
            const bool down = keyIsDown(c) && !shortcutHeld;
            const auto held = held_.find(c);
            if (down && held == held_.end()) {
                const int note = juce::jlimit(0, kMidiMax, baseNote_ + (int) i);
                held_[c] = {note, strip};
                sound(strip, note, true);
                changed = true;
            } else if (!down && held != held_.end()) {
                if (held->second.note >= 0)
                    sound(held->second.strip, held->second.note, false);
                held_.erase(held);
                changed = true;
            }
        }
        return changed;
    }

private:
    static bool typingInTextEditor() {
        return dynamic_cast<juce::TextEditor*>(
                   juce::Component::getCurrentlyFocusedComponent()) != nullptr;
    }

    void releaseAll() {
        for (auto& [c, h] : held_) if (h.note >= 0) sound(h.strip, h.note, false);
        held_.clear();
    }

    void silenceHeld() {
        for (auto& [c, h] : held_) {
            if (h.note >= 0) sound(h.strip, h.note, false);
            h.note = -1;
        }
    }

    void sound(MidiKeyboardStrip* strip, int note, bool on) {
        if (strip != nullptr && MidiKeyboardStrip::isAlive(strip)) {
            MidiKeyboardStrip::qwertyDriving() = true;
            if (on) strip->state().noteOn(1, note, 0.78f);
            else    strip->state().noteOff(1, note, 0.0f);
            MidiKeyboardStrip::qwertyDriving() = false;
            return;
        }
        if (emitDirect)
            emitDirect(on ? juce::MidiMessage::noteOn(1, note, 0.78f)
                          : juce::MidiMessage::noteOff(1, note));
    }

    static constexpr std::string_view kKeys{"awsedftgyhujkolp;"};

public:
    static constexpr int kDefaultBaseNote = 24;

private:
    static constexpr int kMinBaseNote = 12;
    static constexpr int kMaxBaseNote = 96;

    struct HeldNote {
        int note = 0;
        MidiKeyboardStrip* strip = nullptr;
    };

    int baseNote_ = kDefaultBaseNote;
    std::map<char, HeldNote> held_;
    bool enabled_ = AppSettings::instance().getInt("qwertyPiano.enabled", 1) != 0;
};

}
