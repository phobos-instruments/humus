// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <memory>
#include <string>

#include <juce_osc/juce_osc.h>

#include "gui/host/HostCore.h"
#include "gui/host/BrickHost.h"
#include "core/net/OscControl.h"
#include "gui/settings/OscSerial.h"

namespace hum {


class OscHost : private juce::OSCReceiver::Listener<juce::OSCReceiver::MessageLoopCallback> {
public:
    OscHost(BrickHost& host, HostCore& core);
    ~OscHost() override;

    bool setEnabled(bool on);
    bool enabled() const { return enabled_; }
    int port() const;

    juce::String mapAddress(const std::string& address, const std::string& organism,
                    const std::string& param, double min, double max, bool steal);
    void clearAddress(const std::string& address, const std::string& organism,
                      const std::string& param);
    void clearForOrganism(const std::string& organism);
    void renameOrganism(const std::string& oldName, const std::string& newName);
    const OscControlMap& map() const { return map_; }
    void setShape(const std::string& address, const std::string& organism,
                  const std::string& param, const ControlShape& shape);
    std::vector<OscParamUpdate> tickSmoothing(double dt) { return map_.tick(dt); }

    void inject(const std::string& address, double value01);

    juce::String lastAddress() const { return lastAddress_; }
    void clearLastAddress() { lastAddress_.clear(); }

    void syncMapFromModel();
    void syncMapToModel();

    bool sendValue(const juce::String& address, float value);

    void applySerialSettings();
    bool serialEnabled() const { return serial_.enabled(); }
    bool serialOpen() const { return serial_.isOpen(); }
    void takeSerialMessage(const osc::Message& m);

private:
    void oscMessageReceived(const juce::OSCMessage&) override;
    void logToMonitors(bool out, const juce::String& address, const juce::String& args);

    BrickHost& host_;
    HostDocument& doc_;
    juce::OSCReceiver receiver_;
    OscControlMap map_;
    juce::String lastAddress_;
    bool enabled_ = false;
    juce::OSCSender sender_;
    juce::String sendHost_;
    int sendPort_ = 0;
    bool senderOk_ = false;
    OscSerialLink serial_;
    std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);
};

}
