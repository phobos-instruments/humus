// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <memory>
#include <string>
#include <utility>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/bricks/FileTransport.h"
#include "gui/bricks/SoundFileSlot.h"
#include "gui/editor/BrickBindings.h"
#include "gui/host/BrickHost.h"
#include "gui/style/Colours.h"

namespace hum {

class FileBox : public juce::Component {
public:
    static constexpr int kPad = 3;
    static constexpr int kNameH = 16;
    static constexpr int kTransportH = 22;
    static constexpr int kPlainH = kNameH + SoundFileSlot::kIconRowH + 2 * kPad;
    static constexpr int kPlayingH = kPlainH + kTransportH + kPad;

    FileBox(BrickHost& host, const std::string& organism, const std::string& param,
            std::string wildcard, std::string title, std::string kind, const Bindings& bound)
        : slot_(host, organism, param, std::move(wildcard), std::move(title), std::move(kind)) {
        slot_.setStacked(true);
        addAndMakeVisible(slot_);
        if (!bound(bind::kActive).empty()) {
            transport_ = std::make_unique<FileTransport>(host, organism, bound);
            addAndMakeVisible(*transport_);
        }
    }

    SoundFileSlot& slot() { return slot_; }
    bool playableForTest() const { return transport_ != nullptr; }

    void setSwatch(juce::Colour c) { slot_.setSwatch(c); }
    void refresh() { slot_.refresh(); }
    juce::String shownName() const { return slot_.shownName(); }

    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat().reduced(0.5f);
        g.setColour(Palette::panelLight);
        g.fillRoundedRectangle(r, 4.0f);
        g.setColour(Palette::border);
        g.drawRoundedRectangle(r, 4.0f, 1.0f);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(kPad);
        if (transport_ != nullptr && r.getHeight() >= kNameH + SoundFileSlot::kIconRowH + kTransportH) {
            transport_->setBounds(r.removeFromBottom(kTransportH));
            r.removeFromBottom(kPad);
        } else if (transport_ != nullptr) {
            transport_->setVisible(false);
        }
        slot_.setBounds(r);
    }

private:
    SoundFileSlot slot_;
    std::unique_ptr<FileTransport> transport_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileBox)
};

}
