#include "core/BridgedPlugin.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "core/PluginHost.h"

namespace hum {

BridgedPlugin::BridgedPlugin(std::string classRaw, const juce::PluginDescription& desc,
                             const juce::File& workerExe)
    : classRaw_(std::move(classRaw)), client_(std::make_unique<BridgeClient>()) {
    const auto& io = PluginHost::instance().ioFactsFor(classRaw_);
    ins_ = io.ins;
    outs_ = io.outs;
    acceptsMidi_ = io.acceptsMidi;
    producesMidi_ = io.producesMidi;
    hasEditor_ = io.hasEditor;

    for (const auto& d : PluginHost::instance().schemaFor(classRaw_)) {
        Parameter p;
        p.index = (int) params.all().size();
        p.name = d.name;
        p.type = "double";
        p.value = p.rangeMin = p.rangeMax = d.def;
        params.add(p);
    }
    lastSentParams_.assign((size_t) kBridgeMaxParams, -1.0f);
    stagedIn_.assign((size_t) kBridgeMaxMidi, MidiEvent{});
    collected_.assign((size_t) kBridgeMaxMidi, MidiEvent{});
    pendingLive_.reserve(64);

    workerExe_ = workerExe;
    if (auto xml = desc.createXml()) descXml_ = xml->toString();
    std::string err;
    launched_ = client_->launch(workerExe_, classRaw_, descXml_, ins_, outs_, err);
    if (!launched_) responding_.store(false);
    activeClient_.store(client_.get());
}

BridgedPlugin::~BridgedPlugin() { client_->shutdown(); }

void BridgedPlugin::prepare(double sampleRate, int maxBlock) {
    preparedSampleRate_ = sampleRate;
    preparedBlock_ = juce::jmin(maxBlock, kBridgeMaxBlock);
    missLimit_ = juce::jmax(2, (int) std::ceil(2.0 * sampleRate / preparedBlock_));
    if (launched_) client_->sendPrepare(sampleRate, preparedBlock_, {});
    if (client_->helloReceived()) {
        const auto& h = client_->hello();
        acceptsMidi_ = h.acceptsMidi;
        producesMidi_ = h.producesMidi;
        pluginLatency_ = h.latency;
    }
}

void BridgedPlugin::countMiss() {
    if (++misses_ >= missLimit_) responding_.store(false);
}

void BridgedPlugin::respawn() {
    auto fresh = std::make_unique<BridgeClient>();
    fresh->setCachedState(client_->getState(0));
    std::string err;
    if (!fresh->launch(workerExe_, classRaw_, descXml_, ins_, outs_, err)) {
        dead_ = true;
        responding_.store(false);
        return;
    }
    fresh->sendPrepare(preparedSampleRate_, preparedBlock_, {});
    activeClient_.store(fresh.get());
    retired_ = std::move(client_);
    client_ = std::move(fresh);
    responding_.store(true);
    wireEditor();
}

void BridgedPlugin::pollLifecycle() {
    retired_.reset();
    client_->probeChildDeath();
    if (dead_ || !client_->crashed()) return;
    const auto now = juce::Time::getMillisecondCounter();
    if (lastRespawnMs_ != 0 && now - lastRespawnMs_ < 30000) {
        dead_ = true;
        responding_.store(false);
        return;
    }
    lastRespawnMs_ = now;
    respawn();
}

void BridgedPlugin::restart() {
    dead_ = false;
    lastRespawnMs_ = juce::Time::getMillisecondCounter();
    retired_.reset();
    respawn();
}

void BridgedPlugin::deliverMidi(int, const MidiEvent* events, int count) {
    stagedInCount_ = juce::jmin(count, (int) stagedIn_.size());
    for (int i = 0; i < stagedInCount_; ++i) stagedIn_[(size_t) i] = events[i];
}

int BridgedPlugin::collectMidi(int, MidiEvent* out, int capacity) {
    const int n = juce::jmin(collectedCount_, capacity);
    for (int i = 0; i < n; ++i) out[i] = collected_[(size_t) i];
    return n;
}

void BridgedPlugin::queueMidiMessage(const juce::MidiMessage& m) {
    if (m.getRawDataSize() < 1 || m.getRawDataSize() > 3) return;
    MidiEvent e;
    e.sampleOffset = 0;
    for (int i = 0; i < m.getRawDataSize(); ++i) e.data[i] = m.getRawData()[i];
    e.size = m.getRawDataSize();
    const juce::ScopedLock sl(midiLock_);
    if (pendingLive_.size() < 64) pendingLive_.push_back(e);
}

void BridgedPlugin::wireEditor() {
    client_->onEditorCreated = [this](unsigned long xid, int w, int h) {
        if (onEditorCreated_) onEditorCreated_(xid, w, h);
    };
    client_->onSizeChanged = [this](int w, int h) {
        if (onEditorSize_) onEditorSize_(w, h);
    };
    client_->onFloatClosed = [this] {
        editorFloating_ = false;
        if (onEditorFloatClosed_) onEditorFloatClosed_();
    };
    if (editorOpen_) {
        client_->createEditor();
        if (editorFloating_) client_->setFloating(true);
    }
}

void BridgedPlugin::bridgeCreateEditor() {
    editorOpen_ = true;
    client_->createEditor();
}

void BridgedPlugin::bridgeDestroyEditor() {
    editorOpen_ = false;
    editorFloating_ = false;
    client_->destroyEditor();
}

void BridgedPlugin::bridgeSetFloating(bool floating) {
    editorFloating_ = floating;
    if (!editorOpen_) { editorOpen_ = true; client_->createEditor(); }
    client_->setFloating(floating);
}

void BridgedPlugin::bridgeSetEditorCallbacks(
    std::function<void(unsigned long, int, int)> onCreated,
    std::function<void(int, int)> onSize,
    std::function<void()> onFloatClosed) {
    onEditorCreated_ = std::move(onCreated);
    onEditorSize_ = std::move(onSize);
    onEditorFloatClosed_ = std::move(onFloatClosed);
    wireEditor();
}

std::string BridgedPlugin::getStateBase64() const { return client_->getState(); }

void BridgedPlugin::setStateBase64(const std::string& base64) { client_->setState(base64); }

float BridgedPlugin::paramValue(int index) const {
    auto* ac = activeClient_.load();
    auto* h = ac ? ac->ring().header() : nullptr;
    if (h == nullptr || index < 0 || index >= kBridgeMaxParams) return 0.0f;
    return h->paramValues[index].load(std::memory_order_relaxed);
}

void BridgedPlugin::writeSubmitSlot(const float* const* in, int numIn, int numSamples,
                                    const Transport& transport, BridgeClient* c,
                                    BridgeShmHeader* h, uint32_t seq) {
    auto& slot = h->slots[seq % 2];
    const int n = juce::jmin(numSamples, kBridgeMaxBlock);
    slot.numSamples = (uint32_t) n;
    for (int i = 0; i < (int) h->numIns; ++i) {
        float* dst = c->ring().slotAudio((int) (seq % 2), i);
        if (i < numIn && in[i] != nullptr)
            std::memcpy(dst, in[i], sizeof(float) * (size_t) n);
        else
            std::memset(dst, 0, sizeof(float) * (size_t) n);
    }
    int nm = juce::jmin(stagedInCount_, kBridgeMaxMidi);
    for (int i = 0; i < nm; ++i) slot.midiIn[i] = stagedIn_[(size_t) i];
    {
        const juce::ScopedTryLock tl(midiLock_);
        if (tl.isLocked()) {
            for (const auto& e : pendingLive_) {
                if (nm >= kBridgeMaxMidi) break;
                slot.midiIn[nm++] = e;
            }
            pendingLive_.clear();
        }
    }
    slot.numMidiIn = (uint32_t) nm;
    stagedInCount_ = 0;
    uint32_t nc = 0;
    for (const auto& p : params.all()) {
        if (p.index < 0 || p.index >= kBridgeMaxParams) continue;
        const float v = juce::jlimit(0.0f, 1.0f, (float) p.value);
        if (v != lastSentParams_[(size_t) p.index] && nc < (uint32_t) kBridgeMaxParams) {
            slot.paramChanges[nc++] = {(uint32_t) p.index, v};
            lastSentParams_[(size_t) p.index] = v;
        }
    }
    slot.numParamChanges = nc;
    slot.bpm = transport.tempo();
    slot.ppq = transport.beats();
    slot.playing = transport.playing() ? 1u : 0u;
}

void BridgedPlugin::process(const float* const* in, int numIn,
                            float* const* out, int numOut,
                            int numSamples, const Transport& transport) {
    auto silence = [&] {
        for (int o = 0; o < numOut; ++o)
            if (out[o] != nullptr) std::memset(out[o], 0, sizeof(float) * (size_t) numSamples);
        collectedCount_ = 0;
    };

    auto* c = activeClient_.load(std::memory_order_acquire);
    if (c != lastClientSeen_) {
        lastClientSeen_ = c;
        submittedSeq_ = 0;
        misses_ = 0;
        std::fill(lastSentParams_.begin(), lastSentParams_.end(), -1.0f);
    }
    auto* h = c ? c->ring().header() : nullptr;
    if (h == nullptr || c->crashed()
        || h->childReady.load(std::memory_order_acquire) == 0) {
        silence();
        if (c != nullptr && c->crashed()) responding_.store(false);
        return;
    }

    bool copied = false;
    if (submittedSeq_ > 0) {
        const uint32_t want = submittedSeq_;
        if (c->ring().waitAckAtLeast(want, 2)) {
            auto& slot = h->slots[want % 2];
            const int n = juce::jmin((int) slot.numSamples, numSamples);
            for (int o = 0; o < numOut; ++o) {
                if (out[o] == nullptr) continue;
                if (o < (int) h->numOuts) {
                    const float* src = c->ring().slotAudio((int) (want % 2),
                                                           (int) h->numIns + o);
                    std::memcpy(out[o], src, sizeof(float) * (size_t) n);
                    if (n < numSamples)
                        std::memset(out[o] + n, 0, sizeof(float) * (size_t) (numSamples - n));
                } else {
                    std::memset(out[o], 0, sizeof(float) * (size_t) numSamples);
                }
            }
            collectedCount_ = juce::jmin((int) slot.numMidiOut, kBridgeMaxMidi);
            for (int i = 0; i < collectedCount_; ++i) collected_[(size_t) i] = slot.midiOut[i];
            pluginLatency_ = (int) slot.latencySamples;
            copied = true;
            misses_ = 0;
            responding_.store(true);
        }
    }
    if (!copied) {
        silence();
        if (submittedSeq_ > 0) countMiss();
    }

    if (submittedSeq_ - h->ackSeq.load(std::memory_order_acquire) < 2) {
        const uint32_t seq = submittedSeq_ + 1;
        writeSubmitSlot(in, numIn, numSamples, transport, c, h, seq);
        h->reqSeq.store(seq, std::memory_order_release);
        c->ring().postReq();
        submittedSeq_ = seq;
    } else {
        countMiss();
    }
}

}
