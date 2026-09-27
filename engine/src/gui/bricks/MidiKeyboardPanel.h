// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include <juce_audio_utils/juce_audio_utils.h>

#include "gui/common/UiTicker.h"
#include "gui/editor/OrganismEditor.h"
#include "core/graph/GraphMidi.h"
#include "gui/host/BrickHost.h"
#include "hum/caps/Midi.h"

namespace hum {

class MidiKeyboardStrip : public juce::Component,
                          private juce::MidiKeyboardState::Listener {
public:
    explicit MidiKeyboardStrip(BrickHost& host, std::string ownerNode = {})
        : host_(host), owner_(std::move(ownerNode)),
          kb_(state_, juce::MidiKeyboardComponent::horizontalKeyboard) {
        state_.addListener(this);
        kb_.setLowestVisibleKey(kDefaultLowestKey);
        kb_.setKeyWidth(14.0f);
        kb_.setWantsKeyboardFocus(false);
        addAndMakeVisible(kb_);
        registry().push_back(this);
        if (!owner_.empty()) tickerId_ = UiTicker::instance().add([this] { mirrorHeldKeys(); });
    }
    ~MidiKeyboardStrip() override {
        if (tickerId_ != 0) UiTicker::instance().remove(tickerId_);
        state_.allNotesOff(1);
        auto& r = registry();
        r.erase(std::remove(r.begin(), r.end(), this), r.end());
        state_.removeListener(this);
    }

    static constexpr int kDefaultLowestKey = 24;

    static MidiKeyboardStrip* activeStrip() {
        return registry().empty() ? nullptr : registry().back();
    }
    static bool isAlive(MidiKeyboardStrip* s) {
        auto& r = registry();
        return std::find(r.begin(), r.end(), s) != r.end();
    }
    juce::MidiKeyboardState& state() { return state_; }

    static bool& qwertyDriving() { static bool driving = false; return driving; }

    void ensureKeyRangeVisible(int lowNote, int highNote) {
        const auto lo = kb_.getRectangleForKey(lowNote);
        const auto hi = kb_.getRectangleForKey(highNote);
        const bool onScreen = lo.getX() >= 0.0f && hi.getRight() <= (float) kb_.getWidth();
        if (!onScreen) kb_.setLowestVisibleKey(lowNote);
    }

    void resized() override { kb_.setBounds(getLocalBounds()); }

    void mirrorHeldKeys() {
        const auto* keys = dynamic_cast<const HeldKeys*>(host_.liveOrganism(owner_));
        if (keys == nullptr) return;
        const juce::ScopedValueSetter<bool> quiet(mirroring_, true);
        for (int half = 0; half < 2; ++half) {
            const auto now = keys->heldKeys(half);
            const auto changed = now ^ shown_[half];
            for (int b = 0; b < 64; ++b) {
                if (((changed >> b) & 1u) == 0) continue;
                const int note = half * 64 + b;
                if (((now >> b) & 1u) != 0) state_.noteOn(1, note, 1.0f);
                else                        state_.noteOff(1, note, 0.0f);
            }
            shown_[half] = now;
        }
    }

private:
    static std::vector<MidiKeyboardStrip*>& registry() {
        static std::vector<MidiKeyboardStrip*> r;
        return r;
    }

    void handleNoteOn(juce::MidiKeyboardState*, int ch, int note, float vel) override {
        emit(juce::MidiMessage::noteOn(ch, note, vel));
    }
    void handleNoteOff(juce::MidiKeyboardState*, int ch, int note, float vel) override {
        emit(juce::MidiMessage::noteOff(ch, note, vel));
    }
    void emit(const juce::MidiMessage& m) {
        if (mirroring_) return;
        MidiEvent e;
        if (!graphmidi::toEvent(m, e)) return;
        if (owner_.empty() || qwertyDriving()) host_.injectLiveMidi(e);
        else                                   host_.injectLiveMidiToNode(owner_, e);
    }

    BrickHost& host_;
    std::string owner_;
    juce::MidiKeyboardState state_;
    juce::MidiKeyboardComponent kb_;
    int tickerId_ = 0;
    bool mirroring_ = false;
    std::uint64_t shown_[2] = {0, 0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiKeyboardStrip)
};

}
