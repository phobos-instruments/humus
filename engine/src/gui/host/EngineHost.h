// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <map>
#include <set>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_core/juce_core.h>

#include "core/graph/AudioGraph.h"
#include "core/graph/PodModel.h"
#include "core/midi/MidiControl.h"
#include "core/midi/MidiRecord.h"
#include "core/midi/MidiSync.h"
#include "hum/PatternMatrix.h"
#include "hum/caps/Audio.h"
#include "hum/caps/Files.h"
#include "hum/caps/Midi.h"
#include "hum/Parameter.h"
#include "hum/dsp/DcBlock.h"
#include "hum/dsp/DynamicsCore.h"
#include "hum/dsp/LiveWavWriter.h"
#include "io/PatchDocument.h"
#include "io/MediaRefs.h"

#include "gui/host/EngineHostAutomation.h"
#include "gui/host/EngineHostClips.h"
#include "gui/host/LinkSync.h"
#include "gui/host/ParamHistory.h"
#include "gui/host/BrickHost.h"
#include "gui/host/TracksHost.h"
#include "gui/host/PatcherHost.h"
#include "gui/host/EditorHost.h"
#include "gui/host/PropertiesHost.h"
#include "gui/host/PluginsHost.h"
#include "gui/host/SettingsHost.h"
#include "gui/host/VideoHost.h"
#include "gui/host/AssistantHost.h"
#include "gui/host/PodClip.h"
#include "gui/host/ControlCord.h"
#include "gui/host/HostCore.h"
#include "gui/host/MidiState.h"
#include "gui/host/EngineHostDeck.h"
#include "gui/host/EngineHostMetapad.h"
#include "gui/host/EngineHostMidiControl.h"
#include "gui/host/GamepadHost.h"
#include "gui/host/ModHost.h"
#include "gui/host/OscHost.h"
#include "gui/host/EngineHostFiles.h"
#include "gui/host/EngineHostPattern.h"
#include "gui/host/EngineHostPresets.h"
#include "gui/host/EngineHostRecord.h"

#include "hum/dsp/DspMath.h"

namespace juce { class Component; }

namespace hum {

class HostedPlugin;
class PluginNode;

class EngineHost final : public virtual BrickHost,
                         public TracksHost,
                         public PatcherHost,
                         public virtual EditorHost,
                         public PropertiesHost,
                         public virtual PluginsHost,
                         public SettingsHost,
                         public VideoHost,
                         public AssistantHost,
                         private HostCore,
                         private juce::AudioIODeviceCallback,
                         private juce::MidiInputCallback {
public:
    EngineHost();
    ~EngineHost() override;

    void newDocument(juce::Point<int> masterPos = {520, 360});
    bool loadFile(const std::string& path, std::string& error);
    bool saveFile(const std::string& path, std::string& error);
    bool saveCopy(const std::string& path, std::string& error);
    juce::uint64 changeStamp() const override { return changeStamp_; }
    unsigned laneStamp() const { return laneStamp_; }
    unsigned locateStamp() const override { return locateStamp_; }
    double locateBeat() const override { return locateBeat_; }
    unsigned textStamp() const override { return textStamp_; }
    void markDirty() override { dirty_ = true; ++changeStamp_; }

    PatchDocumentModel& model() override { return model_; }
    const PatchDocumentModel& model() const override { return model_; }
    juce::Point<int> position(const std::string& name) const override;
    void setPosition(const std::string& name, juce::Point<int> p) override { positions_[name] = p; }
    juce::Point<int> freeSpot(juce::Point<int> want) const;
    juce::Point<int> spotBelowPatch() const override;

    juce::Point<int> editorPosition(const std::string& name) const override;
    bool editorVisible(const std::string& name) const override;
    void setEditorState(const std::string& name, juce::Point<int> pos, bool visible) override;
    int  editorMode(const std::string& name) const override;
    void setEditorMode(const std::string& name, int mode) override;
    juce::Point<int> editorSize(const std::string& name) const override;
    int editorHalf(const std::string& name) const override;
    void setEditorSize(const std::string& name, juce::Point<int> size, int half) override;
    bool editorCollapsed(const std::string& name) const override;
    void setEditorCollapsed(const std::string& name, bool collapsed) override;
    bool editorFloating(const std::string& name) const override;
    juce::Rectangle<int> editorFloatBounds(const std::string& name) const override;
    void setEditorFloating(const std::string& name, bool floating, juce::Rectangle<int> bounds) override;

    std::string addOrganism(const std::string& className, juce::Point<int> at,
                               const std::string& podScope = {}) override;

    std::string createPod(int ins, int outs, juce::Point<int> at,
                          const std::string& scope = {}, bool stereoPairs = false,
                          int midiIns = 0, int midiOuts = 0) override;
    std::string makePod(const std::vector<std::string>& nodes, juce::Point<int> at,
                        const std::string& scope = {}) override;
    bool renamePod(const std::string& pod, const std::string& newLeaf) override;
    void ungroupPod(const std::string& pod) override;
    void deletePod(const std::string& pod) override;
    void disconnectPod(const std::string& pod) override;
    std::string insertBeforePod(const std::string& pod, const std::string& newClass) override;
    std::string insertAfterPod(const std::string& pod, const std::string& newClass) override;
    PodClip capturePod(const std::string& pod) const override;
    std::string pastePod(const PodClip& clip, juce::Point<int> at,
                         const std::string& scope = {}) override;
    std::string importPatchAsPod(const std::string& path, juce::Point<int> at,
                                 const std::string& scope, std::string& error) override;
    void removeOrganism(const std::string& name) override;
    bool renameOrganism(const std::string& oldName, const std::string& newName) override;
    std::string replaceOrganism(const std::string& name, const std::string& newClass) override;

    std::string substituteOrganism(const std::string& name, const std::string& newClass) override;
    std::string insertBefore(const std::string& name, const std::string& newClass) override;
    std::string insertAfter(const std::string& name, const std::string& newClass) override;
    std::string insertRecorderFor(const std::string& node);
    std::string insertOnCord(const std::string& src, int outlet, const std::string& dst, int inlet,
                             const std::string& newClass, juce::Point<int> at,
                             const std::string& podScope = {}) override;
    void swapOrganisms(const std::string& a, const std::string& b) override;
    void disconnectOrganism(const std::string& name) override;
    void setSoloed(const std::string& name, bool on) override;
    bool soloed(const std::string& name) const override { return solo_.count(name) != 0; }
    bool anySoloed() const { return !solo_.empty(); }
    void applySolo();
    void setSoloable(std::vector<std::string> rows) override { soloable_ = std::move(rows); applySolo(); }

    void setBypass(const std::string& name, bool on) override;
    void setTrackMuted(const std::string& name, bool on) override;
    bool trackMuted(const std::string& name) const override;
    bool bypassed(const std::string& name) const override;
    bool liveBypassed(const std::string& name) const {
        return graph_ != nullptr && graph_->nodeBypass(graph_->indexOf(name));
    }
    std::string missingClassNote(const std::string& name) const override;
    void applyBypass(const std::string& name, bool on);
    void applyTrackMute(const std::string& name, bool on);
    void fireRandom(const std::string& name, bool high);
    void fireTransport(const std::string& action, bool high);
    void firePresetStep(const std::string& name, int dir, bool high);
    void pullVoiceParams(const std::string& name);
    void pullVoiceTexts(const std::string& name) override;
    bool cordWouldStray(const std::string& src, int outlet, const std::string& dst, int inlet,
                        pods::Domain dom = pods::Domain::Audio) const;
    void connect(const std::string& src, int outlet, const std::string& dst, int inlet) override;
    void syncRoutes() override;
    void pushSocketSources();
    using ControlCord = hum::ControlCord;
    int controlInletsOf(const std::string& name) const override;
    bool socketShown(const std::string& name, const std::string& param) const;
    int controlOutletsOf(const std::string& name) override;
    std::string controlInletParam(const std::string& name, int inlet) const override;
    std::string controlOutletValue(const std::string& name, int outlet) override;
    void connectControl(const std::string& src, int outlet, const std::string& dst, int inlet) override;
    void disconnectControl(const std::string& src, int outlet, const std::string& dst, int inlet) override;
    std::vector<ControlCord> controlCords();
    std::vector<ControlCord> controlCordsInScope(const std::string& scope) override;
    bool controlEndpoint(std::string& node, int& pin, bool isDstSide) const;
    void disconnectInto(const std::string& dst, int inlet);
    void removeConnection(const std::string& src, int outlet, const std::string& dst, int inlet) override;
    bool isConnected(const std::string& src, int outlet, const std::string& dst, int inlet) const override;
    void connectMidi(const std::string& src, int srcPort, const std::string& dst, int dstPort) override;
    void disconnectMidiInto(const std::string& dst, int dstPort);
    void removeMidiConnection(const std::string& src, int srcPort, const std::string& dst, int dstPort) override;
    bool isMidiConnected(const std::string& src, int srcPort, const std::string& dst, int dstPort) const override;
    void setMidiCordChannel(const std::string& src, int srcPort,
                            const std::string& dst, int dstPort, int channel) override;
    int midiCordChannel(const std::string& src, int srcPort,
                        const std::string& dst, int dstPort) const override;
    void connectVideo(const std::string& src, int srcPort, const std::string& dst, int dstPort) override;
    void removeVideoConnection(const std::string& src, int srcPort, const std::string& dst, int dstPort) override;
    bool isVideoConnected(const std::string& src, int srcPort, const std::string& dst, int dstPort) const override;
    std::string videoSourceInto(const std::string& dst, int dstPort) const override;
    std::string videoSourceInto(const std::string& dst, int dstPort, int& srcOutlet) const override;

    void setParam(const std::string& organism, const std::string& param, double value) override;
    bool rollLocked(const std::string& organism, const std::string& param) const override {
        const auto* cm = model_.byName(organism);
        return cm != nullptr && cm->rollLocked.count(param) > 0;
    }
    void setRollLocked(const std::string& organism, const std::string& param, bool locked) override;

    void setParamRange(const std::string& organism, const std::string& param,
                       double min, double max) override;
    void setParamText(const std::string& organism, const std::string& param,
                      const std::string& text) override;
    void editParam(const std::string& organism, const std::string& param, double value) override;

    ParamHistory& paramHistory() override { return paramHistory_; }
    std::string duplicateTrack(const std::string& name);
    NodeState captureNodeState(const std::string& name) const override;
    void applyNodeState(const std::string& name, const NodeState& s) override;
    double liveParamValue(const std::string& organism, const std::string& param) override;
    double liveParamMax(const std::string& organism, const std::string& param) override;
    void batchLiveParamValues(const std::string& name,
                              const std::vector<hum::Parameter>& params,
                              std::vector<double>& out) const override;
    std::string liveParamText(const std::string& organism, const std::string& param) override;
    std::string textForDsp(const std::string& organism, const std::string& text) const;
    void beginParamDrag(const std::string& organism, const std::string& param) override;
    void endParamDrag() override;

    FileHost& files() override { return files_; }
    DeckHost& decks() override { return decks_; }
    MidiHost& midi() override { return midi_; }
    const MidiHost& midi() const { return midi_; }
    OscHost& osc() override { return osc_; }
    const OscHost& osc() const { return osc_; }
    ModHost& mod() override { return mod_; }
    const ModHost& mod() const { return mod_; }
    GamepadHost& gamepads() override { return gamepads_; }

    int  midiSyncMode() const override { return syncMode_.load(std::memory_order_relaxed); }
    void setMidiSyncMode(int mode) override;
    void applyMidiSyncFromSettings();

    void setLinkEnabled(bool on);
    bool linkEnabled() const { return linkEnabled_.load(std::memory_order_relaxed); }
    int  linkPeers() const;
    void applyLinkFromSettings();
    void setLinkStartStopSync(bool on);
    bool linkStartStopSync() const { return linkStartStop_.load(std::memory_order_relaxed); }

    static constexpr int kMidiPorts = MidiState::kPorts;
    void pollMidiControl();
    void pollMidiOut();
    void routeLiveMidi(const juce::MidiMessage& m, int port = -1);
    void pushToMonitors(const juce::MidiMessage& m, int port);
    void setLiveTargets(std::vector<std::string> nodes) override {
        const juce::ScopedLock ml(midiState_.targetsLock);
        liveTargets_ = std::move(nodes);
    }
    std::vector<std::string> liveTargets() const {
        const juce::ScopedLock ml(midiState_.targetsLock);
        return liveTargets_;
    }

    void injectLiveMidi(const juce::MidiMessage& m) override;
    void injectLiveMidiToNode(const std::string& node, const juce::MidiMessage& m) override;
    void sendMidiOut(const std::string& organism, const std::string& param, double value);
    void flushMidiRecording(bool finalize = false) override;
    static constexpr int kClipOnDemand = MidiHost::kClipOnDemand;
    void finishOnDemandClips();
    void syncPluginStateToModel();
    HostedPlugin* hostedPluginFor(const std::string& name) override;
    PluginNode* pluginNodeFor(const std::string& name) override;
    Organism* liveOrganism(const std::string& name) override {
        return graph_ ? graph_->find(name) : nullptr;
    }
    void pollBridges();
    void pumpPluginTunings();
    void pollTuningProbes();
    void restartPluginNode(const std::string& name) override;
    std::function<void(const std::function<bool(const std::string&)>& survives)>
        onBeforeRebuild;
    std::function<void()> onTopologyChanged;
    std::function<void(const std::string& error)> onBuildFailed;
    std::function<void()> onArrangementChanged;

    std::function<void(const std::string& organism, const std::string& param)>
        openParameterControl;
    std::function<void(const std::string& organism)> openVisuals;
    void showVisuals(const std::string& organism) override { if (openVisuals) openVisuals(organism); }
    bool canShowParameterControl() const override { return static_cast<bool>(openParameterControl); }
    void showParameterControl(const std::string& organism, const std::string& param) override {
        if (openParameterControl) openParameterControl(organism, param);
    }
    void noteNodeRolled(const std::string& organism) override { if (onNodeRolled) onNodeRolled(organism); }
    std::function<void(std::unique_ptr<juce::Component>)> showCard;
    void presentCard(std::unique_ptr<juce::Component> card) override {
        if (showCard) showCard(std::move(card));
    }
    void rollNode(const std::string& name) override;

    std::function<void(const std::string& organism)> onNodeRolled;
    std::function<void(const std::string& organism)> onPanelEdit;
    void notePanelEdit(const std::string& organism) override { if (onPanelEdit) onPanelEdit(organism); }

    PresetHost& presets() override { return presets_; }

    AutomationHost& automation() override { return automation_; }
    const AutomationHost& automation() const { return automation_; }

    RecordHost& record() override { return record_; }
    void keepLast(int bars);
    const std::string& documentPath() const override { return docPath_; }
    juce::File documentDir() const override;
    std::vector<media::Ref> missingMedia() const;
    int relocateMedia(const media::Ref& located, const juce::File& found);
    bool isLiveTracked(const std::string& organism, const std::string& param) const override;
    bool isExternallyControlled(const std::string& organism,
                               const std::string& param) const override;

    PatternHost& patterns() override { return patterns_; }

    ClipEditor& clips() override { return clips_; }

    MetaEditor& metapad() override { return metapad_; }

    double songLengthBeats() const { return model_.clock.songLength; }
    void setSongLengthBeats(double beats) override;
    double songEndBeat() const override;
    double songEndSeconds() const {
        const double bpm = tempo() > 0.0 ? tempo() : 120.0;
        return songEndBeat() * kSecondsPerMinute / bpm;
    }
    void goToEnd() { setPositionBeats(songEndBeat()); }

    void pushUndo() override;
    void pushParamStep() override;
    void beginTransaction() override;
    void endTransaction() override;
    void requestRebuild();
    unsigned rebuildCount() const { return rebuilds_; }
    bool canUndo() const override { return !undo_.empty(); }
    bool canRedo() const override { return !redo_.empty(); }

    bool isDirty() const { return dirty_; }
    int pendingPatternSyncs() const { return (int) patternPending_.size(); }
    int undoDepth() const { return (int) undo_.size(); }
    bool topUndoIsSnapshot() const { return undo_.empty() || undo_.back().snapshot; }
    int topUndoReverts() const {
        if (undo_.empty()) return 0;
        return (int) (undo_.back().params.size() + undo_.back().patterns.size());
    }
    bool undo() override;
    bool redo() override;

    struct LiveControlScope {
        explicit LiveControlScope(EngineHost& h) : host_(h), prev_(h.liveControl_) { h.liveControl_ = true; }
        ~LiveControlScope() { host_.liveControl_ = prev_; }
        EngineHost& host_;
        bool prev_;
    };

    void holdPatternSync() override { ++patternBatch_; }
    void releasePatternSync() override {
        if (--patternBatch_ > 0) return;
        auto pending = std::move(patternPending_);
        patternPending_.clear();
        for (const auto& n : pending) syncPattern(n);
    }
    using PatternSyncBatch = PatternSyncHold;
    struct DerivedControlScope {
        explicit DerivedControlScope(EngineHost& h) : host_(h), prev_(h.derivedControl_) { h.derivedControl_ = true; }
        ~DerivedControlScope() { host_.derivedControl_ = prev_; }
        EngineHost& host_;
        bool prev_;
    };
    struct NoLatchScope {
        explicit NoLatchScope(EngineHost& h) : host_(h), prev_(h.noLatch_) { h.noLatch_ = true; }
        ~NoLatchScope() { host_.noLatch_ = prev_; }
        EngineHost& host_;
        bool prev_;
    };
    unsigned liveControlGeneration() const { return liveControlGen_; }
    void pokeLiveRefresh() override { ++liveControlGen_; }

    std::string clockNodeName() override;
    std::function<std::string()> noteCaptureHint;
    std::string noteCaptureTarget() const override;
    std::string audioCaptureTarget() const;
    bool commitRetroactiveAudio(double startBeat, double nowBeat, double beats);

    std::string clockNodeNameIfAny() const override {
        for (const auto& cm : model_.organisms)
            if (isClockPseudo(cm.displayClass)) return cm.name;
        return {};
    }

    std::string metapadNodeName() override;
    std::string metapadNodeNameIfAny() const override {
        for (const auto& cm : model_.organisms)
            if (isMetapadPseudo(cm.displayClass)) return cm.name;
        return {};
    }
    double metapadX() const override { return metapad_.x(); }
    double metapadY() const override { return metapad_.y(); }

    void nodeActivity(const std::string& name, float& audioPeak, float& midiCount) override;

    int nodeMeter(const std::string& name, float* levels, int maxCh) override;

    bool audioAlive() const override {
        return juce::Time::getMillisecondCounterHiRes()
                   - lastCallbackMs_.load(std::memory_order_relaxed) < 250.0;
    }

    bool armNodeCapture(int samples) override;
    void disarmNodeCapture() override;
    int copyNodeCapture(const std::string& name, std::vector<float>& out) override;

    bool nodeIsNoteTrack(const std::string& name);
    bool nodeIsInternal(const std::string& name) const {
        const auto* c = model_.byName(name);
        return c != nullptr && c->internal;
    }
    void setNodeInternal(const std::string& name, bool internal) override {
        if (auto* c = mutableByName(name)) c->internal = internal;
    }
    bool nodeArrangesVideo(const std::string& name) override;
    void syncNodeTrack(const std::string& name) override;

    int inletsOf(const std::string& name) override;
    int outletsOf(const std::string& name) override;
    int midiInletsOf(const std::string& name) override;
    int midiOutletsOf(const std::string& name) override;
    int videoInletsOf(const std::string& name) override;
    int videoOutletsOf(const std::string& name) override;

    std::string masterOutputName();
    int connectToMaster(const std::string& node) override;
    std::vector<std::string> arrangeableNodes() override;
    bool nodeRecordsAudio(const std::string& name) override;
    bool nodeRecordsVideo(const std::string& name) override;
    bool nodeRecordsMedia(const std::string& name) override {
        return nodeRecordsAudio(name) || nodeRecordsVideo(name);
    }
    static int reconcilePropertyTypes(PatchDocumentModel& doc);

    void startAudioAsync(std::function<void(bool ok, std::string error)> done = {});
    bool ensureAudio() override;
    void stopAudio();
    bool audioRunning() const { return audioRunning_; }
    bool audioStarting() const { return audioStarting_; }
    juce::AudioDeviceManager& audioDevices() override { return devices_; }
    int maxTrackHeldForTest() const { return graph_ ? graph_->maxTrackHeld() : 0; }
    void resetMaxTrackHeldForTest() { if (graph_) graph_->resetMaxTrackHeld(); }
    void startDeviceForTest(juce::AudioIODevice& d) { audioDeviceAboutToStart(&d); }
    void renderOrganismForTest(const std::string& name, float* const* out, int numOut, int n) {
        const juce::ScopedLock sl(lock_);
        if (auto* c = graph_ ? graph_->find(name) : nullptr)
            c->process(nullptr, 0, out, numOut, n, graph_->transport());
    }
    void injectMidiForTest(const juce::MidiMessage& m) { handleIncomingMidiMessage(nullptr, m); }
    void renderBlockForTest(float* const* out, int numOut, int numSamples) {
        fadeTarget_.store(1.0f);
        audioDeviceIOCallbackWithContext(nullptr, 0, out, numOut, numSamples, {});
    }
    void persistAudioState() override;

    std::vector<std::pair<int, std::string>> choiceItems(const std::string& source,
                                                         const std::string& organism = {}) override;

    void applyPackChanges() { rebuild(); }

    void setOutputGain(float g) {
        g = juce::jlimit(0.0f, 1.0f, g);
        outputGain_.store(g);
        if (std::abs((double) g - model_.masterLevel) > 1e-6) {
            model_.masterLevel = g;
            markDirty();
        }
    }
    float outputGain() const { return outputGain_.load(); }

    void setLimiter(bool on) {
        limiterOn_.store(on, std::memory_order_relaxed);
        if (model_.masterLimiter != on) {
            model_.masterLimiter = on;
            markDirty();
        }
    }
    bool limiterEnabled() const { return limiterOn_.load(std::memory_order_relaxed); }
    float limiterReduction() const { return limiterGr_.load(std::memory_order_relaxed); }

    void setGroove(double amount, const std::string& unit) {
        const double a = juce::jlimit(0.0, 1.0, amount);
        if (model_.groove == a && model_.grooveUnit == unit) return;
        model_.groove = a;
        model_.grooveUnit = unit;
        applyGroove();
        markDirty();
    }
    double groove() const override { return model_.groove; }
    std::string grooveUnit() const override { return model_.grooveUnit; }

    void setMetronome(bool on) { metronome_.store(on, std::memory_order_relaxed); }
    bool metronome() const { return metronome_.load(std::memory_order_relaxed); }
    void setMetronomeGain(float g) { metroGain_.store(juce::jlimit(0.0f, 1.0f, g)); }
    float metronomeGain() const { return metroGain_.load(); }
    using BeforeRecord = hum::BeforeRecord;
    void setBeforeRecord(BeforeRecord mode, int bars) {
        beforeRecord_ = mode;
        beforeRecordBars_ = juce::jlimit(1, 8, bars);
    }
    BeforeRecord beforeRecord() const override { return beforeRecord_; }
    int beforeRecordBars() const override { return beforeRecordBars_; }
    bool countInRunning() const { return countInLeft_.load() > 0; }
    void armCountIn(int bars) override { pendingCountInBars_ = juce::jlimit(1, 8, bars); }
    void armPreRoll(double punchBeat) override { punchInBeat_.store(punchBeat, std::memory_order_relaxed); }
    void cancelPreRoll() override { punchInBeat_.store(-1.0, std::memory_order_relaxed); pendingCountInBars_ = 0; }
    bool preRolling() const override { return punchInBeat_.load(std::memory_order_relaxed) >= 0.0; }
    double punchInBeat() const { return punchInBeat_.load(std::memory_order_relaxed); }
    void serviceCountIn();

    void pumpOscOut();
    double sampleRate() const override { return sampleRate_; }
    int blockSize() const override { return block_; }

    void play() override;
    void playFromStart() override;
    void stop() override;
    void goToStart();
    void setPositionBeats(double beat) override;
    bool isPlaying() const override { return playing_; }
    void setTempo(double bpm);
    void performTempo(double bpm) override;
    double tempo() const override { return model_.clock.tempo; }
    double liveTempo() const { return graph_ ? graph_->transport().tempo() : model_.clock.tempo; }
    double liveGroove() const { return graph_ ? graph_->transport().groove().amount : model_.groove; }
    int liveGrooveGrid() const;
    void performGroove(double amount);
    void performGrooveGrid(int index);
    bool routesAlive() const { return graph_ != nullptr && graph_->modRouteCount() > 0; }

    const std::string& notes() const { return model_.notes; }
    void setNotes(const std::string& s) {
        if (model_.notes == s) return;
        model_.notes = s;
        markDirty();
    }
    int positionBar();
    double positionBeat();
    double positionBeats() override;
    double positionSeconds();
    float outputLevel(int channel) const;

    unsigned dropoutCount() const { return dropouts_.load(std::memory_order_relaxed); }
    float audioLoad() const { return audioLoad_.load(std::memory_order_relaxed); }
    unsigned deviceRestartCount() const { return deviceRestarts_.load(std::memory_order_relaxed); }

    std::string consolidate(const std::string& node, double fromBeat, double toBeat,
                            std::string& error) override;
    std::string bounceSourceOf(const std::string& node) override;
    std::string printToTimeline(const std::string& node, std::string& error, int atTick = -1,
                                const std::string& preferredTarget = {}) override;
    std::vector<std::string> noteTargets(const std::string& node) override;

    bool renderToFile(const std::string& path, double seconds, std::string& error);
    using SoundSink = std::function<bool(const float* const*, int, int)>;
    struct OfflineSound {
        std::unique_ptr<AudioGraph> graph;
        MasterTap* tap = nullptr;
    };
    bool openOfflineSound(OfflineSound& made, std::string& error);
    bool renderOfflineSound(OfflineSound& made, double fromSeconds, double toSeconds,
                            const SoundSink& sink, std::string& error);
    bool renderRange(double fromSeconds, double toSeconds, const SoundSink& sink,
                     std::string& error);
    void primeNoteTracks(AudioGraph& g) const;

    bool startMixRecording(const std::string& path, std::string& error);
    void stopMixRecording();
    bool isMixRecording() const { return mixRecording_.load(); }

    void primeOffline(int blocks) override;
    void takeTransportRequests();
    void advanceModulation(double dt) override;
    void holdAudio(bool held) override;
    bool audioHeld() const { return audioHeld_; }
    bool graphSelfDriven() const { return audioRunning_ && !audioHeld_; }

private:
    MetaEditor metapad_{*this, *this};
    ClipEditor clips_{*this, *this};
    DeckHost decks_{*this, *this};
    PatternHost patterns_{*this, *this};
    PresetHost presets_{*this, *this};
    FileHost files_{*this, *this};
    AutomationHost automation_{*this, *this};
    MidiState midiState_;
    MidiHost midi_{*this, *this, midiState_};
    OscHost osc_{*this, *this};
    ModHost mod_{*this, *this};
    GamepadHost gamepads_{*this};
    RecordHost record_{*this, *this};
    std::string docPath_;

    void onFileNodeChanged(const std::string& organism, const std::string& filePath,
                           bool sourceMoved = true, const std::string& fileParam = "File");
    void autoDetectDeckGrid(const std::string& organism, const std::string& filePath);
    void applyGroove();
    bool setClockGroove(const std::string& organism, const std::string& param, double value);
    void restoreUnautomatedGroove(bool amountLane, bool gridLane);
    void rebuild();
    void applyMidiTrackTarget(const std::string& name, int value);
    void syncMidiTrackTargets();
    void discardLiveGraph();
    void publishClock() override;
    void applyLoadedLayout();
    void syncViewsFromPositions();
    static int countInlets(const PatchDocumentModel&, const std::string& name);
    static int countOutlets(const PatchDocumentModel&, const std::string& name);
    const Organism* liveNode(const std::string& name);
    void dropInvalidConnections();
    void syncAutomation() override;
    void syncMeter() override;
    bool derivedControl() const override { return derivedControl_; }
    void syncPattern(const std::string& name) override;
    void markPatternEdited() override { if (!liveControl_) dirty_ = true; }
    PatchDocumentModel& document() override { return model_; }
    void flagDirty() override { dirty_ = true; }
    void bumpChangeStamp() override { ++changeStamp_; }
    void bumpLiveControl() override { ++liveControlGen_; }
    void bumpLaneStamp() override { ++laneStamp_; }
    AudioGraph* graph() override { return graph_.get(); }
    juce::CriticalSection& graphLock() override { return lock_; }
    bool capturing() const override { return capturing_; }
    void setCapturing(bool on) override { capturing_ = on; }
    Touches& touches() override { return touched_; }
    RecordHost& recorder() override { return record_; }
    const std::vector<std::pair<std::string, Recorder*>>& recorders() const override { return recorders_; }
    void renameReferences(const std::string& oldName, const std::string& newName);
    void promotePodStrays(const std::string& pod, int domain,
                          juce::Point<int> topLeft, juce::Point<int> bottomLeft);
    void splicePodPorts(const std::string& pod, int domain);
    OrganismModel* mutableByName(const std::string& name) override;
    void beginCapturePass() override {
        captureStartBeat_ = positionBeats();
        pinPerformanceControls();
    }
    void pinPerformanceControls();
    double prePassValue(const std::string& organism, const std::string& param);
    void capturePoint(const std::string& organism, const std::string& param,
                      double lo, double hi, bool isRange,
                      double prevLo = std::numeric_limits<double>::quiet_NaN(),
                      double prevHi = std::numeric_limits<double>::quiet_NaN());
    void capturePointAt(const std::string& organism, const std::string& param,
                        double lo, double hi, bool isRange, double beat);
    void noteTouch(const std::string& organism, const std::string& param);
    void clearTouches() override;
    struct CapturePass {
        double firstBeat, lastBeat, highBeat;
        double lo = 0.0, hi = 0.0;
        bool isRange = false;
    };
    bool latch_ = false;
    std::map<std::pair<std::string, std::string>, CapturePass> capturePass_;
    void endCapturePasses() override;
    bool capturing_ = false;
public:
    void extendLatchPasses();
    void setLatchMode(bool on) { latch_ = on; }
    bool latchMode() const { return latch_; }
private:
    unsigned laneStamp_ = 0;
    unsigned locateStamp_ = 0;
    double locateBeat_ = 0.0;

    struct PerfGesture {
        double beat;
        std::string organism, param;
        double value, valueHi;
        bool isRange;
    };
    std::vector<PerfGesture> perfRing_;
    std::set<std::pair<std::string, std::string>> touched_;
    std::vector<float> perfAudioL_, perfAudioR_;
    std::atomic<std::int64_t> perfAudioWrite_{0};
    void trimPerfRings();
    static constexpr double kRingSeconds = 30.0;
    void applyStateAt(double beat);

    void audioDeviceAboutToStart(juce::AudioIODevice*) override;
    void audioDeviceIOCallbackWithContext(const float* const*, int, float* const*, int, int,
                                          const juce::AudioIODeviceCallbackContext&) override;
    void audioDeviceStopped() override {}

    void handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage&) override;

    PatchDocumentModel model_;
    std::map<std::string, juce::Point<int>> positions_;
    std::unique_ptr<juce::XmlElement> original_;

    bool modelSwapped_ = false;
    std::unique_ptr<AudioGraph> graph_;
    MasterTap* soundOut_ = nullptr;
    std::vector<std::pair<MasterTap*, int>> masterTaps_;
    std::vector<std::pair<HardwareOut*, int>> auxOuts_;
    std::vector<std::pair<std::string, Recorder*>> recorders_;
    LiveWavWriter mixWriter_;
    std::atomic<bool> mixRecording_{false};
    std::vector<LiveMidiIn*> liveMidiIns_;
    std::vector<PendingMidiOut*> midiDrains_;
    std::vector<LiveMidiIn*> midiMonitors_;
    std::vector<std::pair<std::string, LiveMidiIn*>> namedLiveIns_;
    std::vector<std::string> liveTargets_;
    std::set<std::string> solo_;
    std::vector<std::string> soloable_;
    std::atomic<double> liveBeats_{0.0};
    std::atomic<double> liveSeconds_{0.0};
    std::atomic<int> liveBar_{1};
    std::atomic<double> liveBeatInBar_{1.0};
    std::atomic<double> tempoReq_{0.0};
    std::atomic<double> seekBeatsReq_{-1.0};
    std::atomic<unsigned> dropouts_{0};
    std::atomic<float> audioLoad_{0.0f};
    std::atomic<unsigned> deviceRestarts_{0};
    juce::CriticalSection lock_;

    struct ParamRevert {
        std::string organism;
        std::string param;
        Parameter before;
        bool existed = true;
    };
    struct PatternRevert {
        std::string organism;
        Pattern before;
    };
    struct UndoState {
        bool snapshot = true;
        PatchDocumentModel model;
        std::map<std::string, juce::Point<int>> positions;
        std::vector<ParamRevert> params;
        std::vector<PatternRevert> patterns;
        bool wasDirty = false;
    };
    static constexpr std::size_t kUndoMaxSteps = 100;
    static constexpr std::size_t kUndoMaxBytes = 64u * 1024u * 1024u;
    static std::size_t approxBytes(const UndoState& s);
    static void trimHistory(std::deque<UndoState>& d);
    std::deque<UndoState> undo_, redo_;
    bool paramStepOpen_ = false;
    void recordParamRevert(const std::string& organism, const std::string& param);
    void recordPatternRevert(const std::string& organism) override;
    UndoState inverseOf(const UndoState& step) const;
    void applyDelta(const UndoState& step);
    bool inTxn_ = false;
    bool txnPushed_ = false;
    int txnDepth_ = 0;
    bool rebuildDue_ = false;
    unsigned rebuilds_ = 0;
    bool dirty_ = false;
    juce::uint64 changeStamp_ = 0;
    unsigned textStamp_ = 0;
    bool writeDocumentTo(const std::string& path, std::string& error,
                         bool pullPluginState = true);
    void storeSessionAudioTo(const std::string& path);
    bool liveControl_ = false;
    bool derivedControl_ = false;
    bool noLatch_ = false;
    unsigned liveControlGen_ = 0;

    std::string paramEditKey_;
    juce::uint32 paramEditTime_ = 0;
    bool paramDragActive_ = false;
    bool paramDragPushed_ = false;

    double sampleRate_ = kDefaultSampleRate;
    int block_ = 512;
    bool monoFold_ = false;
    std::atomic<int> refusedBlocks_{0};
    std::atomic<int> refusedLargest_{0};
    int outputLatency_ = 0;
    double preparedSampleRate_ = 0.0;
    int preparedBlock_ = 0;

    juce::AudioDeviceManager devices_;
    bool audioHeld_ = false;
    bool audioRunning_ = false;
    bool audioStarting_ = false;
    bool cancelStart_ = false;
    static constexpr int kMaxDeviceChannels = 32;
    int inMap_[kMaxDeviceChannels];
    int outMap_[kMaxDeviceChannels];
    const float* physIn_[kMaxDeviceChannels] = {};
    std::vector<std::vector<float>> dcBuf_;
    DcBlock dcBlock_[kMaxDeviceChannels];
    int physInCount_ = 0;
    bool masterFallback_ = true;
    void ensureAuxChannels();
    void scheduleAuxChannelCheck();
    juce::BigInteger auxTriedIn_, auxTriedOut_;
    juce::String auxTriedDevice_;
    void seedDeviceFormatFromSettings();

public:
    static void ignoreSavedDeviceFormat() { ignoreSavedFormat_ = true; }

    static bool headless() { return headless_; }
    static void setHeadless() { headless_ = true; }

    void requestFadeIn();

    void pumpDeviceBlockForChecks(float* const* out, int numOut, int numSamples) {
        audioDeviceIOCallbackWithContext(nullptr, 0, out, numOut, numSamples, {});
    }
    void markAudioRunningForChecks(bool on) { audioRunning_ = on; }

private:
    static inline bool ignoreSavedFormat_ = false;
    static inline bool headless_ = false;
    bool auxCheckPending_ = false;
    std::shared_ptr<bool> hostAlive_ = std::make_shared<bool>(true);
    std::atomic<bool> playing_{false};
    std::atomic<float> level_[2] {0.0f, 0.0f};
    std::atomic<double> lastCallbackMs_ {0.0};

    juce::MidiDeviceListConnection midiDeviceListConn_;
    double ctrlPollMs_ = 0.0;
    double modPollMs_ = 0.0;
    double captureStartBeat_ = 0.0;
    double lastMorphBeat_ = -1.0;

    std::unique_ptr<juce::ChildProcess> probeChild_;
    std::string probeClass_;
    juce::File probeOut_;
    double probeStartMs_ = 0.0;

    struct ClockTimer;
    struct ClockTimerDeleter { void operator()(ClockTimer*) const; };
    std::unique_ptr<ClockTimer, ClockTimerDeleter> clockTimer_;
    std::atomic<int> syncMode_{0};
    MidiClockChase chase_;
    std::atomic<double> chaseBpm_{0.0};
    std::atomic<int> chaseCmd_{0};
    std::atomic<double> chaseSpp_{-1.0};
    void syncSetOutput(juce::MidiOutput* out);
    void setSyncOutput(juce::MidiOutput* out) override { syncSetOutput(out); }
    juce::MidiInputCallback& midiInputSink() override { return *this; }
    bool setLiveControl(bool on) override { return std::exchange(liveControl_, on); }
    bool setDerivedControl(bool on) override { return std::exchange(derivedControl_, on); }
    void noteTopologyChanged() override {
        if (onTopologyChanged) juce::MessageManager::callAsync([cb = onTopologyChanged] { cb(); });
    }
    void syncTransportEvent(bool starting);
    bool syncHandleMidi(const juce::MidiMessage& m);
    void syncApplyChase();

    std::unique_ptr<LinkSync> link_;
    std::atomic<bool> linkEnabled_{false};
    std::atomic<bool> linkAlign_{false};
    std::atomic<bool> linkStartStop_{false};
    bool linkLastSessionPlaying_ = false;
    void linkApplyUi();

    std::atomic<float> fadeTarget_ {0.0f};
    std::atomic<float> fadeGainPub_ {0.0f};
    std::atomic<bool>  fadeRestart_ {false};
    std::atomic<float> outputGain_ {1.0f};
    std::atomic<bool>  limiterOn_ {false};
    std::atomic<float> limiterGr_ {0.0f};
    hum::LimiterCore limiter_;
    ParamHistory paramHistory_;
    std::map<std::string, bool> randomHigh_;
    std::map<std::string, bool> transportHigh_;
    std::map<std::string, bool> presetStepHigh_;
    int patternBatch_ = 0;
    std::set<std::string> patternPending_;
    std::map<std::string, std::vector<float>> oscOutLast_;
    std::atomic<bool> metronome_ {false};
    std::atomic<float> metroGain_ {1.0f};
    double metroNextBeat_ = -1.0;
    int clickRemain_ = 0, clickLen_ = 1;
    double clickPhase_ = 0.0, clickStep_ = 0.0;
    float clickAmp_ = 0.0f;
    BeforeRecord beforeRecord_ = BeforeRecord::None;
    int beforeRecordBars_ = 1;
    int pendingCountInBars_ = 0;
    std::atomic<double> punchInBeat_ {-1.0};
    std::atomic<bool> punchFire_ {false};
    std::atomic<std::int64_t> countInLeft_ {0};
    std::atomic<bool> countInFire_ {false};
    std::int64_t countInTotal_ = 0;
    double countInNextBeat_ = 0.0;
    double countInTick_ = 0.0;
    double countInBar_ = 0.0;
    float fadeGain_ = 0.0f;
};

}
