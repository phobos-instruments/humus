// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <string>
#include <vector>

#include <juce_audio_utils/juce_audio_utils.h>

#include "gui/editor/OrganismEditor.h"
#include "gui/host/BrickHost.h"

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
    }
    ~MidiKeyboardStrip() override {
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
        if (owner_.empty() || qwertyDriving()) host_.injectLiveMidi(m);
        else                                   host_.injectLiveMidiToNode(owner_, m);
    }

    BrickHost& host_;
    std::string owner_;
    juce::MidiKeyboardState state_;
    juce::MidiKeyboardComponent kb_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiKeyboardStrip)
};

}
