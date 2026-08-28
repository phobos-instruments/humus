#pragma once
#include <string>

#include <juce_osc/juce_osc.h>

#include "core/OscControl.h"

namespace hum {

class EngineHost;

class OscHost : private juce::OSCReceiver::Listener<juce::OSCReceiver::MessageLoopCallback> {
public:
    explicit OscHost(EngineHost& host) : host_(host) { receiver_.addListener(this); }
    ~OscHost() override { receiver_.removeListener(this); }

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

private:
    void oscMessageReceived(const juce::OSCMessage&) override;
    void logToMonitors(bool out, const juce::String& address, const juce::String& args);

    EngineHost& host_;
    juce::OSCReceiver receiver_;
    OscControlMap map_;
    juce::String lastAddress_;
    bool enabled_ = false;
    juce::OSCSender sender_;
    juce::String sendHost_;
    int sendPort_ = 0;
    bool senderOk_ = false;
};

}
