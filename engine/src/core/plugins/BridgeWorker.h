// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <atomic>
#include <functional>
#include <memory>
#include <string>

#include <juce_events/juce_events.h>

#include "core/plugins/BridgeRing.h"
#include "core/plugins/HostedPlugin.h"

namespace hum {

class BridgeWorker : public juce::ChildProcessWorker, private juce::Thread {
public:
    BridgeWorker() : juce::Thread("hum-bridge-render") {}
    ~BridgeWorker() override { stopServing(); }

    std::function<void()> onQuit;

    std::function<void()> onCreateEditor;
    std::function<void()> onDestroyEditor;
    std::function<void(bool floating)> onSetFloating;

    juce::AudioPluginInstance* pluginInstance() {
        return plugin_ ? plugin_->instance() : nullptr;
    }
    void sendEditorCreated(unsigned long xid, int w, int h);
    void sendSizeChanged(int w, int h);
    void sendFloatClosed();

private:
    void handleMessageFromCoordinator(const juce::MemoryBlock& mb) override;
    void handleConnectionLost() override { quit(); }

    void prepareOnMessageThread(const juce::String& descXml, const juce::String& shmPath,
                                const juce::String& classRaw, double sr, int maxBlock,
                                const juce::String& stateB64);
    void run() override;
    void stopServing();
    void quit() {
        stopServing();
        if (onQuit) onQuit();
    }

    BridgeRing ring_;
    std::unique_ptr<HostedPlugin> plugin_;
    Transport transport_;
    uint32_t lastSeq_ = 0;
    std::atomic<bool> serving_{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BridgeWorker)
};

}
