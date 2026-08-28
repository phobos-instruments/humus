#include "core/BridgeWorker.h"

#if !JUCE_WINDOWS
#include <unistd.h>
#endif

namespace hum {

void BridgeWorker::handleMessageFromCoordinator(const juce::MemoryBlock& mb) {
    juce::MemoryInputStream is(mb, false);
    switch ((BridgeMsg) is.readInt()) {
        case BridgeMsg::Prepare: {
            const auto descXml = is.readString();
            const auto shmPath = is.readString();
            const auto classRaw = is.readString();
            const double sr = is.readDouble();
            const int maxBlock = is.readInt();
            const auto state = is.readString();
            juce::MessageManager::callAsync([this, descXml, shmPath, classRaw, sr, maxBlock, state] {
                prepareOnMessageThread(descXml, shmPath, classRaw, sr, maxBlock, state);
            });
            break;
        }
        case BridgeMsg::GetState:
            juce::MessageManager::callAsync([this] {
                const juce::String b64(plugin_ ? plugin_->getStateBase64() : std::string());
                sendMessageToCoordinator(bridgeMsg(BridgeMsg::State, [&](juce::MemoryOutputStream& os) {
                    os.writeString(b64);
                }));
            });
            break;
        case BridgeMsg::SetState: {
            const auto b64 = is.readString();
            juce::MessageManager::callAsync([this, b64] {
                if (plugin_) plugin_->setStateBase64(b64.toStdString());
            });
            break;
        }
        case BridgeMsg::CreateEditor:
            juce::MessageManager::callAsync([this] { if (onCreateEditor) onCreateEditor(); });
            break;
        case BridgeMsg::DestroyEditor:
            juce::MessageManager::callAsync([this] { if (onDestroyEditor) onDestroyEditor(); });
            break;
        case BridgeMsg::SetFloating: {
            const bool floating = is.readBool();
            juce::MessageManager::callAsync([this, floating] {
                if (onSetFloating) onSetFloating(floating);
            });
            break;
        }
        case BridgeMsg::Quit:
            juce::MessageManager::callAsync([this] { quit(); });
            break;
        default:
            break;
    }
}

void BridgeWorker::sendEditorCreated(unsigned long xid, int w, int h) {
    sendMessageToCoordinator(bridgeMsg(BridgeMsg::EditorCreated, [&](juce::MemoryOutputStream& os) {
        os.writeInt64((juce::int64) xid);
        os.writeInt(w);
        os.writeInt(h);
    }));
}

void BridgeWorker::sendSizeChanged(int w, int h) {
    sendMessageToCoordinator(bridgeMsg(BridgeMsg::SizeChanged, [&](juce::MemoryOutputStream& os) {
        os.writeInt(w);
        os.writeInt(h);
    }));
}

void BridgeWorker::sendFloatClosed() {
    sendMessageToCoordinator(bridgeMsg(BridgeMsg::FloatClosed));
}

void BridgeWorker::prepareOnMessageThread(const juce::String& descXml, const juce::String& shmPath,
                                          const juce::String& classRaw, double sr, int maxBlock,
                                          const juce::String& stateB64) {
    const bool rePrepare = plugin_ != nullptr;
    if (rePrepare) {
        if (ring_.header()) ring_.header()->childReady.store(0);
        stopServing();
    }

    std::string err;
    if (!rePrepare) {
        if (!ring_.open(juce::File(shmPath), err)) { quit(); return; }
        sendMessageToCoordinator(bridgeMsg(BridgeMsg::MappedOk));

        auto descElem = juce::parseXML(descXml);
        juce::PluginDescription desc;
        if (descElem == nullptr || !desc.loadFromXml(*descElem)) { quit(); return; }
        juce::AudioPluginFormatManager formats;
        formats.addDefaultFormats();
        juce::String ierr;
        auto inst = formats.createPluginInstance(desc, sr, maxBlock, ierr);
        if (inst == nullptr) { quit(); return; }
        plugin_ = std::make_unique<HostedPlugin>(std::move(inst), classRaw.toStdString());
        if (plugin_->params.all().empty()) {
            const auto& ps = plugin_->instance()->getParameters();
            const int n = juce::jmin((int) ps.size(), kBridgeMaxParams);
            for (int i = 0; i < n; ++i) {
                Parameter p;
                p.index = i;
                p.name = "bridge#" + std::to_string(i);
                p.type = "double";
                p.value = p.rangeMin = p.rangeMax = ps[(size_t) i]->getValue();
                plugin_->params.add(p);
            }
        }
        if (stateB64.isNotEmpty()) plugin_->setStateBase64(stateB64.toStdString());

        BridgeHello hello;
#if JUCE_WINDOWS
        hello.pid = 0;
#else
        hello.pid = (int) ::getpid();
#endif
        hello.ins = plugin_->numAudioInputs();
        hello.outs = plugin_->numAudioOutputs();
        hello.paramCount = (int) plugin_->instance()->getParameters().size();
        hello.acceptsMidi = plugin_->numMidiInputs() > 0;
        hello.producesMidi = plugin_->numMidiOutputs() > 0;
        hello.latency = plugin_->latencySamples();
        sendMessageToCoordinator(bridgeMsg(BridgeMsg::Hello, [&](juce::MemoryOutputStream& os) {
            os.writeInt(hello.pid);
            os.writeInt(hello.ins);
            os.writeInt(hello.outs);
            os.writeInt(hello.latency);
            os.writeInt(hello.paramCount);
            os.writeBool(hello.acceptsMidi);
            os.writeBool(hello.producesMidi);
        }));
    }

    transport_.prepare(sr, 120.0);
    plugin_->prepare(sr, juce::jmin(maxBlock, kBridgeMaxBlock));

    serving_.store(true);
    startThread(juce::Thread::Priority::highest);
    if (ring_.header()) ring_.header()->childReady.store(1);
}

void BridgeWorker::stopServing() {
    serving_.store(false);
    stopThread(2000);
}

void BridgeWorker::run() {
    auto* h = ring_.header();
    if (h == nullptr) return;
    const int ins = (int) h->numIns, outs = (int) h->numOuts;
    const float* inPtrs[64];
    float* outPtrs[64];

    while (!threadShouldExit() && serving_.load()) {
        if (!ring_.waitReqAbove(lastSeq_, 200)) continue;
        const uint32_t seq = h->reqSeq.load(std::memory_order_acquire);
        auto& slot = h->slots[seq % 2];
        const int n = juce::jmin((int) slot.numSamples, kBridgeMaxBlock);

        const int nChanges = juce::jmin((int) slot.numParamChanges, kBridgeMaxParams);
        for (int i = 0; i < nChanges; ++i) {
            const auto& ch = slot.paramChanges[i];
            if (auto* p = plugin_->params.byIndex((int) ch.index)) {
                p->value = ch.value;
                p->rangeMin = p->rangeMax = ch.value;
            }
        }

        transport_.setTempo(slot.bpm);
        transport_.setPlaying(slot.playing != 0);
        transport_.setBeatPosition(slot.ppq);

        for (int i = 0; i < ins; ++i) inPtrs[i] = ring_.slotAudio((int) (seq % 2), i);
        for (int o = 0; o < outs; ++o) outPtrs[o] = ring_.slotAudio((int) (seq % 2), ins + o);

        plugin_->deliverMidi(0, slot.midiIn, juce::jmin((int) slot.numMidiIn, kBridgeMaxMidi));
        plugin_->process(inPtrs, ins, outPtrs, outs, n, transport_);
        slot.numMidiOut = (uint32_t) plugin_->collectMidi(0, slot.midiOut, kBridgeMaxMidi);
        slot.latencySamples = (uint32_t) plugin_->latencySamples();

        const auto& ps = plugin_->instance()->getParameters();
        const int np = juce::jmin((int) ps.size(), kBridgeMaxParams);
        for (int i = 0; i < np; ++i)
            h->paramValues[i].store(ps[i]->getValue(), std::memory_order_relaxed);

        h->ackSeq.store(seq, std::memory_order_release);
        ring_.postAck();
        lastSeq_ = seq;
    }
}

}
