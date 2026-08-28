#pragma once
#include <memory>
#include <string>

#include <juce_core/juce_core.h>

#include "core/BridgeProtocol.h"

namespace hum {

class BridgeRing {
public:
    BridgeRing() = default;
    ~BridgeRing() { close(); }

    bool create(const juce::File& path, int ins, int outs, std::string& err);
    bool open(const juce::File& path, std::string& err);
    void close();

    BridgeShmHeader* header() const { return header_; }
    float* slotAudio(int slot, int plane) const { return bridgeSlotAudio(header_, slot, plane); }
    const juce::File& file() const { return file_; }

    void postReq();
    bool waitAckAtLeast(uint32_t seq, int timeoutMs);
    void postAck();
    bool waitReqAbove(uint32_t lastSeen, int timeoutMs);

    static juce::File preferredDir();

    static void sweepStale(const juce::File& dir = preferredDir());

private:
    std::unique_ptr<juce::MemoryMappedFile> map_;
    juce::File file_;
    BridgeShmHeader* header_ = nullptr;
    bool owner_ = false;
};

}
