// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/settings/OscNetRows.h"

#include "gui/app/AppSettings.h"
#include "gui/common/Localisation.h"
#include "gui/host/OscEdits.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

namespace {

constexpr int kInputPort = 9000;
constexpr int kOutputPort = 9001;

int portFrom(const juce::TextEditor& field, int fallback) {
    const int p = field.getText().trim().getIntValue();
    return p > 0 && p < 65536 ? p : fallback;
}

juce::String sendAddressFrom(const juce::TextEditor& field) {
    const auto text = field.getText().trim();
    return text.isEmpty() ? juce::String("127.0.0.1") : text;
}

}

juce::StringArray thisComputerAddresses() {
    juce::StringArray out;
    for (const auto& a : juce::IPAddress::getAllAddresses(false)) {
        const auto text = a.toString();
        if (a.isNull() || a == juce::IPAddress::local() || text.startsWith("169.254.")) continue;
        out.addIfNotAlreadyThere(text);
    }
    return out;
}

OscInputRow::OscInputRow(SettingsHost* host) : host_(host) {
    auto& s = AppSettings::instance();
    enable_.setButtonText(tr("midi-settings.osc-input-udp", "OSC input (UDP)"));
    enable_.setTooltip(tr("midi-settings.osc-input-tip",
                          "Humus listens on every network address of this computer. Point your OSC "
                          "sender at the address shown here, on this port."));
    enable_.setToggleState(s.getInt("osc.enabled", 0) != 0, juce::dontSendNotification);
    enable_.onClick = [this] { toggle(); };
    addAndMakeVisible(enable_);

    portLabel_.setText(tr("midi-settings.port", "Port"), juce::dontSendNotification);
    addAndMakeVisible(portLabel_);
    port_.setText(juce::String(s.getInt("osc.port", kInputPort)), juce::dontSendNotification);
    port_.setInputRestrictions(5, "0123456789");
    port_.onFocusLost = [this] { commitPort(); };
    port_.onReturnKey = [this] { commitPort(); };
    addAndMakeVisible(port_);

    status_.setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(status_);
    refresh();
}

void OscInputRow::resized() {
    auto r = getLocalBounds();
    enable_.setBounds(r.removeFromLeft(150));
    portLabel_.setBounds(r.removeFromLeft(44));
    port_.setBounds(r.removeFromLeft(70));
    r.removeFromLeft(12);
    status_.setBounds(r);
}

void OscInputRow::toggle() {
    const bool on = enable_.getToggleState();
    AppSettings::instance().set("osc.enabled", on ? 1 : 0);
    if (host_ != nullptr) host_->osc().setEnabled(on);
    refresh();
}

void OscInputRow::commitPort() {
    auto& s = AppSettings::instance();
    const int before = s.getInt("osc.port", kInputPort);
    const int p = portFrom(port_, before);
    port_.setText(juce::String(p), juce::dontSendNotification);
    s.set("osc.port", p);
    const bool retry = p != before || (host_ != nullptr && !host_->osc().enabled());
    if (host_ != nullptr && enable_.getToggleState() && retry) host_->osc().setEnabled(true);
    refresh();
}

void OscInputRow::refresh() {
    const bool listening = host_ != nullptr && host_->osc().enabled();
    const bool busy = host_ != nullptr && enable_.getToggleState() && !listening;
    const auto addresses = thisComputerAddresses();
    const auto where = addresses.isEmpty()
                           ? tr("midi-settings.osc-no-network", "127.0.0.1, no network")
                           : addresses.joinIntoString(", ");
    juce::String text;
    if (busy)
        text = tr("midi-settings.osc-port-busy", "Not listening: port {port} is in use by another app",
                  {{"port", juce::String(host_->osc().port())}});
    else if (listening)
        text = tr("midi-settings.osc-listening", "Listening on {address}", {{"address", where}});
    else
        text = tr("midi-settings.osc-this-computer", "This computer: {address}", {{"address", where}});
    status_.setColour(juce::Label::textColourId, busy ? Palette::warnAmber() : Palette::textDim);
    if (status_.getText() != text) status_.setText(text, juce::dontSendNotification);
}

OscOutputRow::OscOutputRow() {
    auto& s = AppSettings::instance();
    title_.setText(tr("midi-settings.osc-output", "OSC output"), juce::dontSendNotification);
    title_.setTooltip(tr("midi-settings.osc-output-tip",
                         "Where Humus sends OSC. Organisms with OSC switched on, such as Hands and "
                         "Skeleton, send their values to this address and port."));
    addAndMakeVisible(title_);
    toLabel_.setText(tr("midi-settings.osc-send-to", "Send to"), juce::dontSendNotification);
    addAndMakeVisible(toLabel_);
    const auto saved = s.getString("osc.sendHost").trim();
    address_.setText(saved.isEmpty() ? juce::String("127.0.0.1") : saved, juce::dontSendNotification);
    address_.setInputRestrictions(253, "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.-:");
    address_.onFocusLost = [this] { commit(); };
    address_.onReturnKey = [this] { commit(); };
    addAndMakeVisible(address_);
    portLabel_.setText(tr("midi-settings.port", "Port"), juce::dontSendNotification);
    addAndMakeVisible(portLabel_);
    port_.setText(juce::String(s.getInt("osc.sendPort", kOutputPort)), juce::dontSendNotification);
    port_.setInputRestrictions(5, "0123456789");
    port_.onFocusLost = [this] { commit(); };
    port_.onReturnKey = [this] { commit(); };
    addAndMakeVisible(port_);
}

void OscOutputRow::resized() {
    auto r = getLocalBounds();
    title_.setBounds(r.removeFromLeft(150));
    toLabel_.setBounds(r.removeFromLeft(76));
    address_.setBounds(r.removeFromLeft(140));
    r.removeFromLeft(8);
    portLabel_.setBounds(r.removeFromLeft(44));
    port_.setBounds(r.removeFromLeft(70));
}

void OscOutputRow::commit() {
    auto& s = AppSettings::instance();
    const auto address = sendAddressFrom(address_);
    const int p = portFrom(port_, s.getInt("osc.sendPort", kOutputPort));
    address_.setText(address, juce::dontSendNotification);
    port_.setText(juce::String(p), juce::dontSendNotification);
    s.set("osc.sendHost", address);
    s.set("osc.sendPort", p);
}

}
