#pragma once
#include <array>
#include <memory>
#include <vector>

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "core/MidiControl.h"
#include "gui/AppSettings.h"
#include "gui/EngineHost.h"
#include "gui/LookAndFeel.h"

namespace hum {

class MidiSettingsView : public juce::Component, private juce::Timer {
public:
    explicit MidiSettingsView(EngineHost* host) : host_(host) {
        title_.setText("MIDI & OSC", juce::dontSendNotification);
        title_.setFont(juce::FontOptions(16.0f).withStyle("Bold"));
        addAndMakeVisible(title_);

        inputsLabel_.setText("MIDI Inputs", juce::dontSendNotification);
        inputsLabel_.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
        addAndMakeVisible(inputsLabel_);
        outputLabel_.setText("MIDI Outputs", juce::dontSendNotification);
        outputLabel_.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
        addAndMakeVisible(outputLabel_);
        for (int p = 0; p < EngineHost::kMidiPorts; ++p) {
            auto initRow = [this, p](PortRow& row, bool isInput) {
                row.label.setText((isInput ? "MidiIn" : "MidiOut") + juce::String(p + 1),
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

        btMidi_.setButtonText("Bluetooth MIDI devices...");
        btMidi_.onClick = [] { juce::BluetoothMidiDevicePairingDialogue::open(); };
        if (juce::BluetoothMidiDevicePairingDialogue::isAvailable())
            addAndMakeVisible(btMidi_);

        hint_.setColour(juce::Label::textColourId, Palette::textDim);
        hint_.setFont(juce::FontOptions(12.0f));
        hint_.setJustificationType(juce::Justification::topLeft);
        addAndMakeVisible(hint_);

        syncLabel_.setText("Clock sync", juce::dontSendNotification);
        addAndMakeVisible(syncLabel_);
        syncCombo_.addItem("Off", 1);
        syncCombo_.addItem("Generate (send clock to MidiOut1)", 2);
        syncCombo_.addItem("Chase (follow incoming clock)", 3);
        {
            const auto s = AppSettings::instance().getString("midi.sync", "off");
            syncCombo_.setSelectedId(s == "generate" ? 2 : s == "chase" ? 3 : 1,
                                     juce::dontSendNotification);
        }
        addAndMakeVisible(syncCombo_);
        syncCombo_.onChange = [this] {
            const int id = syncCombo_.getSelectedId();
            const int mode = id == 2 ? EngineHost::kSyncGenerate
                           : id == 3 ? EngineHost::kSyncChase : EngineHost::kSyncOff;
            if (host_ != nullptr) host_->setMidiSyncMode(mode);
            else AppSettings::instance().set("midi.sync", id == 2 ? "generate"
                                                          : id == 3 ? "chase" : "off");
        };

        oscEnable_.setButtonText("OSC input (UDP)");
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
                    oscPortLabel_.setText("port in use?", juce::dontSendNotification);
                }
            }
        };
        oscPortLabel_.setText("Port", juce::dontSendNotification);
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

        padEnable_.setButtonText("Game controllers");
        padEnable_.setToggleState(AppSettings::instance().getInt("gamepad.enabled", 1) != 0,
                                  juce::dontSendNotification);
        padEnable_.onClick = [this] {
            const bool on = padEnable_.getToggleState();
            AppSettings::instance().set("gamepad.enabled", on ? 1 : 0);
            if (host_ != nullptr) host_->gamepads().setEnabled(on);
        };
        addAndMakeVisible(padEnable_);

        fineLabel_.setText("Fine drag (hold Shift)", juce::dontSendNotification);
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

        mapLabel_.setText("Mappings (right-click any knob -> MIDI / OSC Learn)",
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

    void resized() override {
        auto r = getLocalBounds().reduced(16, 12);
        title_.setBounds(r.removeFromTop(26));
        r.removeFromTop(10);
        auto grids = r.removeFromTop(18 + 4 + EngineHost::kMidiPorts * 24);
        auto left = grids.removeFromLeft(grids.getWidth() / 2 - 8);
        grids.removeFromLeft(16);
        auto& right = grids;
        inputsLabel_.setBounds(left.removeFromTop(18));
        outputLabel_.setBounds(right.removeFromTop(18));
        left.removeFromTop(4);
        right.removeFromTop(4);
        for (int p = 0; p < EngineHost::kMidiPorts; ++p) {
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

private:
    void applyToHost() {
        if (host_ != nullptr && host_->midi().enabled()) host_->midi().refreshDevices();
    }

    void rebuildDeviceLists() {
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
            row.combo.addItem("(none)", 1);
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
        for (int p = 0; p < EngineHost::kMidiPorts; ++p) {
            fillRow(inRows_[(size_t) p], true, p);
            fillRow(outRows_[(size_t) p], false, p);
        }

        hint_.setText("A MidiIn organism receives its port's device; a MidiOut "
                      "organism sends to its port's device. Changes apply while "
                      "MIDI is enabled (the toolbar MIDI button).",
                      juce::dontSendNotification);
        resized();
        repaint();
    }

    struct MapRow {
        std::unique_ptr<juce::Label> text;
        std::unique_ptr<juce::TextButton> remove;
    };

    void timerCallback() override {
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

    void rebuildMappings() {
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
        for (const auto& e : entries)
            addRow(juce::String(midiSourceLabel(e.cc)) + "   " + juce::String(e.organism)
                       + " / " + juce::String(e.param) + "   (" + juce::String(e.min, 2)
                       + " .. " + juce::String(e.max, 2) + ")",
                   [this, cc = e.cc, c = e.organism, p = e.param] {
                       if (host_ != nullptr) host_->midi().clearCC(cc, c, p);
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
            row.text->setText("none yet", juce::dontSendNotification);
            row.text->setColour(juce::Label::textColourId, Palette::textDim);
            row.text->setFont(juce::FontOptions(12.0f));
            mapRows_.addAndMakeVisible(*row.text);
            mapRowWidgets_.push_back(std::move(row));
        }
        layoutMappingRows();
    }

    void layoutMappingRows() {
        const int w = juce::jmax(100, mapViewport_.getWidth() - 12);
        int y = 0;
        for (auto& row : mapRowWidgets_) {
            if (row.remove) {
                row.remove->setBounds(w - 70, y + 1, 66, 20);
                row.text->setBounds(0, y, w - 76, 22);
            } else {
                row.text->setBounds(0, y, w, 22);
            }
            y += 24;
        }
        mapRows_.setSize(w, juce::jmax(y, 1));
    }

    struct PortRow {
        juce::Label label;
        juce::ComboBox combo;
    };

    EngineHost* host_;
    juce::Label title_, inputsLabel_, outputLabel_, hint_, mapLabel_, oscPortLabel_,
                syncLabel_;
    juce::ComboBox syncCombo_;
    juce::TextButton btMidi_;
    std::array<PortRow, EngineHost::kMidiPorts> inRows_, outRows_;
    std::vector<juce::MidiDeviceInfo> inputs_, outputs_;
    juce::ToggleButton oscEnable_;
    juce::TextEditor oscPort_;
    juce::ToggleButton padEnable_;
    juce::Label padStatus_;
    juce::Label fineLabel_;
    juce::Slider fine_;
    juce::Viewport mapViewport_;
    juce::Component mapRows_;
    std::vector<MapRow> mapRowWidgets_;
    size_t lastMapCount_ = ~(size_t) 0;
    juce::MidiDeviceListConnection listConn_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiSettingsView)
};

}
