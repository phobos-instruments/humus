#pragma once
#include <string>

#include <juce_audio_basics/juce_audio_basics.h>

namespace hum {

class PluginNode {
public:
    virtual ~PluginNode() = default;

    virtual const std::string& classRaw() const = 0;

    virtual std::string getStateBase64() const = 0;
    virtual void setStateBase64(const std::string& base64) = 0;

    virtual void queueMidiMessage(const juce::MidiMessage& m) = 0;

    virtual bool hasEditor() const = 0;

    virtual float paramValue(int index) const = 0;

    virtual bool responding() const { return true; }
};

}
