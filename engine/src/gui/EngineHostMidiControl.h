#pragma once
#include <string>

#include "core/ControlShape.h"

namespace hum {

class EngineHost;
class MidiControlMap;

class MidiHost {
public:
    explicit MidiHost(EngineHost& host) : host_(host) {}

    bool setEnabled(bool on);
    bool enabled() const;
    void refreshDevices();

    juce::String mapCC(int cc, const std::string& organism, const std::string& param,
               double min, double max, bool steal);
    void setShape(int cc, const std::string& organism, const std::string& param,
                  const ControlShape& shape);
    void clearCC(int cc, const std::string& organism, const std::string& param);
    void clearForOrganism(const std::string& organism);
    const MidiControlMap& map() const;
    int  lastCC() const;
    void clearLastCC();

    void setReceiveMode(const std::string& name, int mode, int channel);

    void setRecordTarget(const std::string& name, bool armed, int quantizeTicks = 0,
                         int clip = -1, bool thru = false);
    bool isRecordTarget(const std::string& name) const;
    bool anyRecordTarget() const;

    void syncMapFromModel();
    void syncMapToModel();

private:
    void openConfiguredDevices();
    void migratePortSettings();
    EngineHost& host_;
};

}
