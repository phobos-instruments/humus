#pragma once
#include <atomic>
#include <memory>
#include <string>

#include <functional>

#include <juce_events/juce_events.h>

#include "core/BridgeProtocol.h"
#include "core/BridgeRing.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class BridgeClient : private juce::ChildProcessCoordinator {
public:
    static constexpr const char* kUid = "humBridge";

    BridgeClient() = default;
    ~BridgeClient() override { shutdown(); }

    bool launch(const juce::File& workerExe, const std::string& classRaw,
                const juce::String& descXml, int ins, int outs, std::string& err);

    void sendPrepare(double sampleRate, int maxBlock, const std::string& stateBase64);

    std::string getState(int timeoutMs = 500);
    void setState(const std::string& base64);
    void setCachedState(const std::string& base64) {
        const juce::ScopedLock sl(stateLock_);
        cachedState_ = base64;
    }

    void createEditor();
    void destroyEditor();
    void setFloating(bool floating);
    std::function<void(unsigned long xid, int w, int h)> onEditorCreated;
    std::function<void(int w, int h)> onSizeChanged;
    std::function<void()> onFloatClosed;

    void shutdown();

    BridgeRing& ring() { return ring_; }
    bool crashed() const { return crashed_.load() || childDead_.load(); }
    void probeChildDeath();
    const BridgeHello& hello() const { return hello_; }
    bool helloReceived() const { return helloReceived_.load(); }
    int childPid() const { return hello_.pid; }

private:
    void handleMessageFromWorker(const juce::MemoryBlock& mb) override;
    void handleConnectionLost() override { crashed_.store(true); }

    std::string cachedStateCopy() const {
        const juce::ScopedLock sl(stateLock_);
        return cachedState_;
    }

    BridgeRing ring_;
    juce::File shmFile_;
    std::string classRaw_;
    juce::String descXml_;
    double pendingSr_ = kDefaultSampleRate;
    int pendingBlock_ = 512;

    BridgeHello hello_;
    std::atomic<bool> helloReceived_{false};
    std::atomic<bool> crashed_{false};
    std::atomic<bool> childDead_{false};
    bool shmUnlinked_ = false;

    mutable juce::CriticalSection stateLock_;
    std::string cachedState_;
    std::string stateReply_;
    juce::WaitableEvent stateEvent_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BridgeClient)
};

}
