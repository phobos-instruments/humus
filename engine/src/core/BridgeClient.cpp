#include "core/BridgeClient.h"

#if !JUCE_WINDOWS
#include <signal.h>
#include <unistd.h>
#endif

namespace hum {

static int currentPid() {
#if JUCE_WINDOWS
    return 0;
#else
    return (int) ::getpid();
#endif
}

static bool pidAlive(int pid) {
    if (pid <= 0) return false;
#if JUCE_WINDOWS
    return true;
#else
    const juce::File stat("/proc/" + juce::String(pid) + "/stat");
    if (!stat.existsAsFile()) return false;
    const auto text = stat.loadFileAsString();
    const int close = text.lastIndexOfChar(')');
    const auto state = close >= 0 ? text.substring(close + 1).trim().substring(0, 1)
                                  : juce::String();
    return state != "Z" && state != "X" && state != "x";
#endif
}

void BridgeClient::probeChildDeath() {
    if (!crashed_.load() && helloReceived_.load() && !pidAlive(hello_.pid))
        childDead_.store(true);
}

bool BridgeClient::launch(const juce::File& workerExe, const std::string& classRaw,
                          const juce::String& descXml, int ins, int outs, std::string& err) {
    classRaw_ = classRaw;
    descXml_ = descXml;

    static std::atomic<int> seq{0};
    shmFile_ = BridgeRing::preferredDir().getChildFile(
        "hum-bridge-" + juce::String(currentPid()) + "-" + juce::String(++seq) + ".shm");
    if (!ring_.create(shmFile_, ins, outs, err)) return false;

    if (!launchWorkerProcess(workerExe, kUid, 5000)) {
        err = "bridge: failed to launch worker " + workerExe.getFullPathName().toStdString();
        ring_.close();
        shmFile_.deleteFile();
        return false;
    }
    return true;
}

void BridgeClient::sendPrepare(double sampleRate, int maxBlock, const std::string& stateBase64) {
    pendingSr_ = sampleRate;
    pendingBlock_ = maxBlock;
    if (!stateBase64.empty()) setCachedState(stateBase64);
    sendMessageToWorker(bridgeMsg(BridgeMsg::Prepare, [&](juce::MemoryOutputStream& os) {
        os.writeString(descXml_);
        os.writeString(shmFile_.getFullPathName());
        os.writeString(juce::String(classRaw_));
        os.writeDouble(sampleRate);
        os.writeInt(maxBlock);
        os.writeString(juce::String(stateBase64.empty() ? cachedStateCopy() : stateBase64));
    }));
}

std::string BridgeClient::getState(int timeoutMs) {
    if (!crashed() && helloReceived_.load()) {
        stateEvent_.reset();
        sendMessageToWorker(bridgeMsg(BridgeMsg::GetState));
        if (stateEvent_.wait(timeoutMs)) {
            const juce::ScopedLock sl(stateLock_);
            cachedState_ = stateReply_;
            return stateReply_;
        }
    }
    return cachedStateCopy();
}

void BridgeClient::setState(const std::string& base64) {
    setCachedState(base64);
    sendMessageToWorker(bridgeMsg(BridgeMsg::SetState, [&](juce::MemoryOutputStream& os) {
        os.writeString(juce::String(base64));
    }));
}

void BridgeClient::createEditor()  { sendMessageToWorker(bridgeMsg(BridgeMsg::CreateEditor)); }
void BridgeClient::destroyEditor() { sendMessageToWorker(bridgeMsg(BridgeMsg::DestroyEditor)); }
void BridgeClient::setFloating(bool floating) {
    sendMessageToWorker(bridgeMsg(BridgeMsg::SetFloating, [&](juce::MemoryOutputStream& os) {
        os.writeBool(floating);
    }));
}

void BridgeClient::shutdown() {
    if (!crashed_.load())
        sendMessageToWorker(bridgeMsg(BridgeMsg::Quit));
    killWorkerProcess();
    ring_.close();
    if (!shmUnlinked_) shmFile_.deleteFile();
}

void BridgeClient::handleMessageFromWorker(const juce::MemoryBlock& mb) {
    juce::MemoryInputStream is(mb, false);
    switch ((BridgeMsg) is.readInt()) {
        case BridgeMsg::Hello: {
            hello_.pid = is.readInt();
            hello_.ins = is.readInt();
            hello_.outs = is.readInt();
            hello_.latency = is.readInt();
            hello_.paramCount = is.readInt();
            hello_.acceptsMidi = is.readBool();
            hello_.producesMidi = is.readBool();
            helloReceived_.store(true);
            break;
        }
        case BridgeMsg::MappedOk:
            shmFile_.deleteFile();
            shmUnlinked_ = true;
            break;
        case BridgeMsg::State: {
            {
                const juce::ScopedLock sl(stateLock_);
                stateReply_ = is.readString().toStdString();
            }
            stateEvent_.signal();
            break;
        }
        case BridgeMsg::EditorCreated: {
            const auto xid = (unsigned long) is.readInt64();
            const int w = is.readInt(), h = is.readInt();
            juce::MessageManager::callAsync([cb = onEditorCreated, xid, w, h] {
                if (cb) cb(xid, w, h);
            });
            break;
        }
        case BridgeMsg::SizeChanged: {
            const int w = is.readInt(), h = is.readInt();
            juce::MessageManager::callAsync([cb = onSizeChanged, w, h] {
                if (cb) cb(w, h);
            });
            break;
        }
        case BridgeMsg::FloatClosed:
            juce::MessageManager::callAsync([cb = onFloatClosed] { if (cb) cb(); });
            break;
        default:
            break;
    }
}

}
