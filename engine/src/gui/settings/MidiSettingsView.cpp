// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/settings/MidiSettingsView.h"
#include "gui/host/EngineHostMidiControl.h"
#include "gui/host/GamepadHost.h"

namespace hum {

MidiSettingsView::MidiSettingsView(SettingsHost* host)
    : host_(host) {
    title_.setText(tr("midi-settings.midi-and-osc", "MIDI & OSC"), juce::dontSendNotification);
    title_.setFont(juce::FontOptions(16.0f).withStyle("Bold"));
    addAndMakeVisible(title_);

    inputsLabel_.setText(tr("midi-settings.midi-inputs", "MIDI Inputs"), juce::dontSendNotification);
    inputsLabel_.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
    addAndMakeVisible(inputsLabel_);
    outputLabel_.setText(tr("midi-settings.midi-outputs", "MIDI Outputs"), juce::dontSendNotification);
    outputLabel_.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
    addAndMakeVisible(outputLabel_);
    for (int p = 0; p < MidiState::kPorts; ++p) {
        auto initRow = [this, p](PortRow& row, bool isInput) {
            row.label.setText((isInput ? tr("midi-settings.midiin", "MidiIn") : tr("midi-settings.midiout", "MidiOut")) + juce::String(p + 1),
                              juce::dontSendNotification);
            row.label.setFont(juce::FontOptions(12.0f));
            row.label.setColour(juce::Label::textColourId, Palette::textDim);
            addAndMakeVisible(row.label);
            addAndMakeVisible(row.combo);
            row.combo.onChange = [this, p, isInput, rowPtr = &row] {
                const auto key = juce::String(isInput ? "midi.in." : "midi.out.")
                               + juce::String(p + 1);
                const int id = rowPtr->combo.getSelectedId();
                juce::String ident, name;
                if (id >= 100) {
                    const auto& devs = isInput ? inputs_ : outputs_;
                    if (id - 100 < (int) devs.size()) {
                        ident = devs[(size_t) (id - 100)].identifier;
                        name = devs[(size_t) (id - 100)].name;
                    }
                } else if (id == 2) {
                    return;
                }
                AppSettings::instance().set(key, ident);
                AppSettings::instance().set(key + ".name", name);
                applyToHost();
            };
        };
        initRow(inRows_[(size_t) p], true);
        initRow(outRows_[(size_t) p], false);
    }

    btMidi_.setButtonText(tr("midi-settings.bluetooth-midi-devices", "Bluetooth MIDI devices..."));
    btMidi_.onClick = [] { juce::BluetoothMidiDevicePairingDialogue::open(); };
    if (juce::BluetoothMidiDevicePairingDialogue::isAvailable())
        addAndMakeVisible(btMidi_);

    hint_.setColour(juce::Label::textColourId, Palette::textDim);
    hint_.setFont(juce::FontOptions(12.0f));
    hint_.setJustificationType(juce::Justification::topLeft);
    addAndMakeVisible(hint_);

    syncLabel_.setText(tr("midi-settings.clock-sync", "Clock sync"), juce::dontSendNotification);
    addAndMakeVisible(syncLabel_);
    syncCombo_.addItem(tr("midi-settings.off", "Off"), 1);
    syncCombo_.addItem(tr("midi-settings.generate-send-clock-to-midiout1", "Generate (send clock to MidiOut1)"), 2);
    syncCombo_.addItem(tr("midi-settings.chase-follow-incoming-clock", "Chase (follow incoming clock)"), 3);
    {
        const auto s = AppSettings::instance().getString("midi.sync", "off");
        syncCombo_.setSelectedId(s == "generate" ? 2 : s == "chase" ? 3 : 1,
                                 juce::dontSendNotification);
    }
    addAndMakeVisible(syncCombo_);
    syncCombo_.onChange = [this] {
        const int id = syncCombo_.getSelectedId();
        const int mode = id == 2 ? SettingsHost::kSyncGenerate
                       : id == 3 ? SettingsHost::kSyncChase : SettingsHost::kSyncOff;
        if (host_ != nullptr) host_->setMidiSyncMode(mode);
        else AppSettings::instance().set("midi.sync", id == 2 ? "generate"
                                                      : id == 3 ? "chase" : "off");
    };

    oscEnable_.setButtonText(tr("midi-settings.osc-input-udp", "OSC input (UDP)"));
    oscEnable_.setToggleState(AppSettings::instance().getInt("osc.enabled", 0) != 0,
                              juce::dontSendNotification);
    addAndMakeVisible(oscEnable_);
    oscEnable_.onClick = [this] {
        AppSettings::instance().set("osc.enabled", oscEnable_.getToggleState() ? 1 : 0);
        if (host_ != nullptr) {
            const bool ok = host_->osc().setEnabled(oscEnable_.getToggleState());
            if (oscEnable_.getToggleState() && !ok) {
                oscEnable_.setToggleState(false, juce::dontSendNotification);
                AppSettings::instance().set("osc.enabled", 0);
                oscPortLabel_.setText(tr("midi-settings.port-in-use", "port in use?"), juce::dontSendNotification);
            }
        }
    };
    oscPortLabel_.setText(tr("midi-settings.port", "Port"), juce::dontSendNotification);
    addAndMakeVisible(oscPortLabel_);
    oscPort_.setText(juce::String(AppSettings::instance().getInt("osc.port", 9000)),
                     juce::dontSendNotification);
    oscPort_.setInputRestrictions(5, "0123456789");
    addAndMakeVisible(oscPort_);
    auto commitPort = [this] {
        AppSettings::instance().set("osc.port", oscPort_.getText().getIntValue());
        if (host_ != nullptr && host_->osc().enabled()) host_->osc().setEnabled(true);
    };
    oscPort_.onFocusLost = commitPort;
    oscPort_.onReturnKey = commitPort;

    addAndMakeVisible(oscSerial_);

    padEnable_.setButtonText(tr("midi-settings.game-controllers", "Game controllers"));
    padEnable_.setToggleState(AppSettings::instance().getInt("gamepad.enabled", 1) != 0,
                              juce::dontSendNotification);
    padEnable_.onClick = [this] {
        const bool on = padEnable_.getToggleState();
        AppSettings::instance().set("gamepad.enabled", on ? 1 : 0);
        if (host_ != nullptr) host_->gamepads().setEnabled(on);
    };
    addAndMakeVisible(padEnable_);

    fineLabel_.setText(tr("midi-settings.fine-drag-hold-shift", "Fine drag (hold Shift)"), juce::dontSendNotification);
    addAndMakeVisible(fineLabel_);
    fine_.setSliderStyle(juce::Slider::LinearHorizontal);
    fine_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    fine_.setRange(2.0, 50.0, 1.0);
    fine_.setDoubleClickReturnValue(true, 10.0);
    fine_.setTextValueSuffix("x");
    fine_.setValue((double) AppSettings::instance().getInt("controls.fineDrag", 10),
                   juce::dontSendNotification);
    fine_.onValueChange = [this] {
        AppSettings::instance().set("controls.fineDrag", (int) fine_.getValue());
    };
    addAndMakeVisible(fine_);

    padStatus_.setColour(juce::Label::textColourId, Palette::textDim);
    padStatus_.setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(padStatus_);

    mapLabel_.setText(tr("midi-settings.mappings-right-click-any-knob", "Mappings (right-click any knob -> MIDI / OSC Learn; hold a button while learning for a shift combo)"),
                      juce::dontSendNotification);
    mapLabel_.setColour(juce::Label::textColourId, Palette::textDim);
    mapLabel_.setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(mapLabel_);
    mapViewport_.setViewedComponent(&mapRows_, false);
    mapViewport_.setScrollBarsShown(true, false);
    addAndMakeVisible(mapViewport_);

    rebuildDeviceLists();
    rebuildMappings();
    listConn_ = juce::MidiDeviceListConnection::make([this] { rebuildDeviceLists(); });
    startTimerHz(2);
}

void MidiSettingsView::resized() {
    auto r = getLocalBounds().reduced(16, 12);
    title_.setBounds(r.removeFromTop(26));
    r.removeFromTop(10);
    auto grids = r.removeFromTop(18 + 4 + MidiState::kPorts * 24);
    auto left = grids.removeFromLeft(grids.getWidth() / 2 - 8);
    grids.removeFromLeft(16);
    auto& right = grids;
    inputsLabel_.setBounds(left.removeFromTop(18));
    outputLabel_.setBounds(right.removeFromTop(18));
    left.removeFromTop(4);
    right.removeFromTop(4);
    for (int p = 0; p < MidiState::kPorts; ++p) {
        auto lrow = left.removeFromTop(24);
        inRows_[(size_t) p].label.setBounds(lrow.removeFromLeft(64));
        inRows_[(size_t) p].combo.setBounds(lrow.reduced(0, 1));
        auto rrow = right.removeFromTop(24);
        outRows_[(size_t) p].label.setBounds(rrow.removeFromLeft(64));
        outRows_[(size_t) p].combo.setBounds(rrow.reduced(0, 1));
    }
    if (btMidi_.isVisible()) {
        r.removeFromTop(8);
        btMidi_.setBounds(r.removeFromTop(24).removeFromLeft(190));
    }
    r.removeFromTop(12);
    auto syncRow = r.removeFromTop(26);
    syncLabel_.setBounds(syncRow.removeFromLeft(80));
    syncCombo_.setBounds(syncRow.removeFromLeft(260));
    r.removeFromTop(12);
    hint_.setBounds(r.removeFromTop(34));
    r.removeFromTop(8);
    auto oscRow = r.removeFromTop(24);
    oscEnable_.setBounds(oscRow.removeFromLeft(150));
    oscPortLabel_.setBounds(oscRow.removeFromLeft(80));
    oscPort_.setBounds(oscRow.removeFromLeft(70));
    r.removeFromTop(4);
    oscSerial_.setBounds(r.removeFromTop(24));
    r.removeFromTop(4);
    auto padRow = r.removeFromTop(24);
    padEnable_.setBounds(padRow.removeFromLeft(150));
    padStatus_.setBounds(padRow);
    r.removeFromTop(4);
    auto fineRow = r.removeFromTop(24);
    fineLabel_.setBounds(fineRow.removeFromLeft(150));
    fine_.setBounds(fineRow.removeFromLeft(220));
    r.removeFromTop(10);
    mapLabel_.setBounds(r.removeFromTop(18));
    r.removeFromTop(4);
    mapViewport_.setBounds(r);
    layoutMappingRows();
}

void MidiSettingsView::applyToHost() {
    if (host_ != nullptr && host_->midi().enabled()) host_->midi().refreshDevices();
}

void MidiSettingsView::rebuildDeviceLists() {
    inputs_.clear();
    for (const auto& dev : juce::MidiInput::getAvailableDevices()) inputs_.push_back(dev);
    outputs_.clear();
    for (const auto& dev : juce::MidiOutput::getAvailableDevices()) outputs_.push_back(dev);

    auto fillRow = [this](PortRow& row, bool isInput, int p) {
        const auto& devs = isInput ? inputs_ : outputs_;
        const auto key = juce::String(isInput ? "midi.in." : "midi.out.")
                       + juce::String(p + 1);
        const auto want = AppSettings::instance().getString(key);
        row.combo.clear(juce::dontSendNotification);
        row.combo.addItem(tr("midi-settings.none", "(none)"), 1);
        int selected = 1;
        for (int i = 0; i < (int) devs.size(); ++i) {
            row.combo.addItem(devs[(size_t) i].name, 100 + i);
            if (devs[(size_t) i].identifier == want) selected = 100 + i;
        }
        if (want.isNotEmpty() && selected == 1) {
            auto saved = AppSettings::instance().getString(key + ".name");
            row.combo.addItem((saved.isNotEmpty() ? saved + " " : juce::String())
                                  + "(not connected)", 2);
            selected = 2;
        }
        row.combo.setSelectedId(selected, juce::dontSendNotification);
    };
    for (int p = 0; p < MidiState::kPorts; ++p) {
        fillRow(inRows_[(size_t) p], true, p);
        fillRow(outRows_[(size_t) p], false, p);
    }

    hint_.setText(tr("midi-settings.port-devices",
                     "A MidiIn organism receives its port's device; a MidiOut "
                     "organism sends to its port's device. Changes apply while "
                     "MIDI is enabled (the toolbar MIDI button)."),
                  juce::dontSendNotification);
    resized();
    repaint();
}

void MidiSettingsView::timerCallback() {
    if (!isShowing() || host_ == nullptr) return;
    const size_t count = host_->midi().map().entries().size()
                       + host_->osc().map().entries().size();
    if (count != lastMapCount_) rebuildMappings();
    padStatus_.setText(
        host_->gamepads().enabled()
            ? juce::String::fromUTF8(host_->gamepads().statusText().c_str())
                  + juce::String(" - OSC Learn + move an axis to map")
            : juce::String("off"),
        juce::dontSendNotification);
}

void MidiSettingsView::rebuildMappings() {
    mapRowWidgets_.clear();
    mapLabel_.setVisible(host_ != nullptr);
    mapViewport_.setVisible(host_ != nullptr);
    if (host_ == nullptr) return;
    const auto& entries = host_->midi().map().entries();
    const auto& oscEntries = host_->osc().map().entries();
    lastMapCount_ = entries.size() + oscEntries.size();
    auto addRow = [this](const juce::String& text, std::function<void()> remove) {
        MapRow row;
        row.text = std::make_unique<juce::Label>();
        row.text->setText(text, juce::dontSendNotification);
        row.text->setFont(juce::FontOptions(12.0f));
        mapRows_.addAndMakeVisible(*row.text);
        row.remove = std::make_unique<juce::TextButton>("Remove");
        row.remove->onClick = [this, remove = std::move(remove)] {
            remove();
            rebuildMappings();
        };
        mapRows_.addAndMakeVisible(*row.remove);
        mapRowWidgets_.push_back(std::move(row));
    };
    for (const auto& m : host_->midi().map().modifiers()) addModifierRow(m);
    for (const auto& e : entries)
        addRow(juce::String(midiSourceLabel(e.source())) + "   " + juce::String(e.organism)
                   + " / " + juce::String(e.param) + "   (" + juce::String(e.min, 2)
                   + " .. " + juce::String(e.max, 2) + ")",
               [this, src = e.source(), c = e.organism, p = e.param] {
                   if (host_ != nullptr) host_->midi().clearCC(src, c, p);
               });
    for (const auto& e : oscEntries)
        addRow("OSC " + juce::String(e.address) + "   " + juce::String(e.organism)
                   + " / " + juce::String(e.param) + "   (" + juce::String(e.min, 2)
                   + " .. " + juce::String(e.max, 2) + ")",
               [this, a = e.address, c = e.organism, p = e.param] {
                   if (host_ != nullptr) host_->osc().clearAddress(a, c, p);
               });
    if (entries.empty() && oscEntries.empty()) {
        MapRow row;
        row.text = std::make_unique<juce::Label>();
        row.text->setText(tr("midi-settings.none-yet", "none yet"), juce::dontSendNotification);
        row.text->setColour(juce::Label::textColourId, Palette::textDim);
        row.text->setFont(juce::FontOptions(12.0f));
        mapRows_.addAndMakeVisible(*row.text);
        mapRowWidgets_.push_back(std::move(row));
    }
    layoutMappingRows();
}

void MidiSettingsView::addModifierRow(const MidiModifier& m) {
    MapRow row;
    row.text = std::make_unique<juce::Label>();
    row.text->setText(juce::String(midiSourceLabel(m.source)) + tr("midi-settings.shift-button", "   shift button"),
                      juce::dontSendNotification);
    row.text->setFont(juce::FontOptions(12.0f));
    mapRows_.addAndMakeVisible(*row.text);
    row.latching = std::make_unique<juce::ToggleButton>("Latching");
    row.latching->setToggleState(m.latching, juce::dontSendNotification);
    row.latching->setTooltip(tr("midi-settings.tap-once-to-switch-the", "Tap once to switch the bank on, tap again to switch it off"));
    row.ownAction = std::make_unique<juce::ToggleButton>(tr("midi-settings.own-action", "Own action"));
    row.ownAction->setToggleState(m.ownAction, juce::dontSendNotification);
    row.ownAction->setTooltip(tr("midi-settings.also-fire-this-button-s", "Also fire this button's own mapping when it is pressed"));
    auto apply = [this, source = m.source, latch = row.latching.get(),
                  own = row.ownAction.get()] {
        if (host_ != nullptr)
            host_->midi().setModifier(source, latch->getToggleState(), own->getToggleState());
    };
    row.latching->onClick = apply;
    row.ownAction->onClick = apply;
    mapRows_.addAndMakeVisible(*row.latching);
    mapRows_.addAndMakeVisible(*row.ownAction);
    mapRowWidgets_.push_back(std::move(row));
}

void MidiSettingsView::layoutMappingRows() {
    const int w = juce::jmax(100, mapViewport_.getWidth() - 12);
    int y = 0;
    for (auto& row : mapRowWidgets_) {
        if (row.latching) {
            row.ownAction->setBounds(w - 100, y + 1, 100, 20);
            row.latching->setBounds(w - 190, y + 1, 86, 20);
            row.text->setBounds(0, y, w - 196, 22);
        } else if (row.remove) {
            row.remove->setBounds(w - 70, y + 1, 66, 20);
            row.text->setBounds(0, y, w - 76, 22);
        } else {
            row.text->setBounds(0, y, w, 22);
        }
        y += 24;
    }
    mapRows_.setSize(w, juce::jmax(y, 1));
}

}
