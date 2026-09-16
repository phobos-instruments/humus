// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <memory>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/host/BrickHost.h"
#include "gui/common/Localisation.h"
#include "gui/editor/OrganismEditor.h"
#include "gui/bricks/RiffFileImport.h"

namespace hum {

class BasslineImportBrick : public OrganismEditor {
public:
    BasslineImportBrick(BrickHost& host, std::string name, const juce::String& label)
        : host_(host), name_(std::move(name)) {
        button_.setButtonText(label.isNotEmpty() ? label
                                                 : juce::String::fromUTF8("Load riffs\xe2\x80\xa6"));
        button_.setTooltip(tr("bassline-import.tooltip", "Pattern dumps, .seq or MIDI files"));
        button_.onClick = [this] { choose(); };
        addAndMakeVisible(button_);
    }

    std::function<void()> onLoaded;

    void choose() {
        chooseRiffFiles(host_, name_, chooser_,
                        [safe = juce::Component::SafePointer<BasslineImportBrick>(this)] {
            if (safe != nullptr && safe->onLoaded) safe->onLoaded();
        });
    }

    void reloadValues() override {}
    void refreshAutomatedValues() override {}
    int preferredContentWidth() const override { return 80; }
    int preferredContentHeight(int) const override { return 24; }
    void resized() override { button_.setBounds(getLocalBounds()); }

private:
    BrickHost& host_;
    std::string name_;
    juce::TextButton button_;
    std::unique_ptr<juce::FileChooser> chooser_;
};

}
