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
#include <vector>

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_core/juce_core.h>

#include "core/AudioGraph.h"
#include "core/PodModel.h"
#include "core/MidiControl.h"
#include "core/MidiRecord.h"
#include "core/MidiSync.h"
#include "hum/PatternMatrix.h"
#include "hum/Capabilities.h"
#include "hum/Parameter.h"
#include "hum/dsp/DcBlock.h"
#include "hum/dsp/DynamicsCore.h"
#include "hum/dsp/LiveWavWriter.h"
#include "io/PatchDocument.h"

#include "gui/EngineHostAutomation.h"
#include "gui/EngineHostClips.h"
#include "gui/LinkSync.h"
#include "gui/ParamHistory.h"
#include "gui/EngineHostDeck.h"
#include "gui/EngineHostMetapad.h"
#include "gui/EngineHostMidiControl.h"
#include "gui/GamepadHost.h"
#include "gui/ModHost.h"
#include "gui/OscHost.h"
#include "gui/EngineHostFiles.h"
#include "gui/EngineHostPattern.h"
#include "gui/EngineHostPresets.h"
#include "gui/EngineHostRecord.h"

namespace hum {

class HostedPlugin;
class PluginNode;

class EngineHost : private juce::AudioIODeviceCallback,
                   private juce::MidiInputCallback {
public:
    EngineHost();
    ~EngineHost() override;

    void newDocument(juce::Point<int> masterPos = {520, 360});
    bool loadFile(const std::string& path, std::string& error);
    bool saveFile(const std::string& path, std::string& error);
    bool saveCopy(const std::string& path, std::string& error);
    juce::uint64 changeStamp() const { return changeStamp_; }
    unsigned laneStamp() const { return laneStamp_; }
    unsigned textStamp() const { return textStamp_; }
    void markDirty() { dirty_ = true; ++changeStamp_; }

    PatchDocumentModel& model() { return model_; }
    juce::Point<int> position(const std::string& name) const;
    void setPosition(const std::string& name, juce::Point<int> p) { positions_[name] = p; }
    juce::Point<int> freeSpot(juce::Point<int> want) const;
    juce::Point<int> spotBelowPatch() const;

    juce::Point<int> editorPosition(const std::string& name) const;
    bool editorVisible(const std::string& name) const;
    void setEditorState(const std::string& name, juce::Point<int> pos, bool visible);
    int  editorMode(const std::string& name) const;
    void setEditorMode(const std::string& name, int mode);
    juce::Point<int> editorSize(const std::string& name) const;
    int editorHalf(const std::string& name) const;
    void setEditorSize(const std::string& name, juce::Point<int> size, int half);
    bool editorCollapsed(const std::string& name) const;
    void setEditorCollapsed(const std::string& name, bool collapsed);

    std::string addOrganism(const std::string& className, juce::Point<int> at,
                               const std::string& podScope = {});

    struct PodClip {
        std::string leaf;
        std::vector<OrganismModel> nodes;
        std::vector<ConnectionModel> cords, midiCords, videoCords;
        std::vector<std::pair<std::string, juce::Point<int>>> layout;
        juce::Point<int> boxPos;
    };
    std::string createPod(int ins, int outs, juce::Point<int> at,
                          const std::string& scope = {}, bool stereoPairs = false,
                          int midiIns = 0, int midiOuts = 0);
    std::string makePod(const std::vector<std::string>& nodes, juce::Point<int> at,
                        const std::string& scope = {});
    bool renamePod(const std::string& pod, const std::string& newLeaf);
    void ungroupPod(const std::string& pod);
    void deletePod(const std::string& pod);
    void disconnectPod(const std::string& pod);
    std::string insertBeforePod(const std::string& pod, const std::string& newClass);
    std::string insertAfterPod(const std::string& pod, const std::string& newClass);
    PodClip capturePod(const std::string& pod) const;
    std::string pastePod(const PodClip& clip, juce::Point<int> at,
                         const std::string& scope = {});
    std::string importPatchAsPod(const std::string& path, juce::Point<int> at,
                                 const std::string& scope, std::string& error);
    void removeOrganism(const std::string& name);
    bool renameOrganism(const std::string& oldName, const std::string& newName);
    std::string replaceOrganism(const std::string& name, const std::string& newClass);

    std::string substituteOrganism(const std::string& name, const std::string& newClass);
    std::string insertBefore(const std::string& name, const std::string& newClass);
    std::string insertAfter(const std::string& name, const std::string& newClass);
    std::string insertRecorderFor(const std::string& node);
    std::string insertOnCord(const std::string& src, int outlet, const std::string& dst, int inlet,
                             const std::string& newClass, juce::Point<int> at,
                             const std::string& podScope = {});
    void swapOrganisms(const std::string& a, const std::string& b);
    void disconnectOrganism(const std::string& name);
    void setSoloed(const std::string& name, bool on);
    bool soloed(const std::string& name) const { return solo_.count(name) != 0; }
    bool anySoloed() const { return !solo_.empty(); }
    void applySolo();
    void setSoloable(std::vector<std::string> rows) { soloable_ = std::move(rows); applySolo(); }

    void setBypass(const std::string& name, bool on);
    void setTrackMuted(const std::string& name, bool on);
    bool trackMuted(const std::string& name) const;
    bool bypassed(const std::string& name) const;
    bool liveBypassed(const std::string& name) const {
        return graph_ != nullptr && graph_->nodeBypass(graph_->indexOf(name));
    }
    std::string missingClassNote(const std::string& name) const;
    void applyBypass(const std::string& name, bool on);
    void applyTrackMute(const std::string& name, bool on);
    void fireRandom(const std::string& name, bool high);
    void fireTransport(const std::string& action, bool high);
    void firePresetStep(const std::string& name, int dir, bool high);
    void applyMetapadTarget(const std::string& param, double value);
    void pullVoiceParams(const std::string& name);
    void pullVoiceTexts(const std::string& name);
    bool cordWouldStray(const std::string& src, int outlet, const std::string& dst, int inlet,
                        pods::Domain dom = pods::Domain::Audio) const;
    void connect(const std::string& src, int outlet, const std::string& dst, int inlet);
    void disconnectInto(const std::string& dst, int inlet);
    void removeConnection(const std::string& src, int outlet, const std::string& dst, int inlet);
    bool isConnected(const std::string& src, int outlet, const std::string& dst, int inlet) const;
    void connectMidi(const std::string& src, int srcPort, const std::string& dst, int dstPort);
    void disconnectMidiInto(const std::string& dst, int dstPort);
    void removeMidiConnection(const std::string& src, int srcPort, const std::string& dst, int dstPort);
    bool isMidiConnected(const std::string& src, int srcPort, const std::string& dst, int dstPort) const;
    void setMidiCordChannel(const std::string& src, int srcPort,
                            const std::string& dst, int dstPort, int channel);
    int midiCordChannel(const std::string& src, int srcPort,
                        const std::string& dst, int dstPort) const;
    void connectVideo(const std::string& src, int srcPort, const std::string& dst, int dstPort);
    void removeVideoConnection(const std::string& src, int srcPort, const std::string& dst, int dstPort);
    bool isVideoConnected(const std::string& src, int srcPort, const std::string& dst, int dstPort) const;
    std::string videoSourceInto(const std::string& dst, int dstPort) const;

    void setParam(const std::string& organism, const std::string& param, double value);
    bool rollLocked(const std::string& organism, const std::string& param) const {
        const auto* cm = model_.byName(organism);
        return cm != nullptr && cm->rollLocked.count(param) > 0;
    }
    void setRollLocked(const std::string& organism, const std::string& param, bool locked);

    void setParamRange(const std::string& organism, const std::string& param,
                       double min, double max);
    void setParamText(const std::string& organism, const std::string& param,
                      const std::string& text);
    void editParam(const std::string& organism, const std::string& param, double value);

    ParamHistory& paramHistory() { return paramHistory_; }
    std::string duplicateTrack(const std::string& name);
    NodeState captureNodeState(const std::string& name) const;
    void applyNodeState(const std::string& name, const NodeState& s);
    double liveParamValue(const std::string& organism, const std::string& param);
    double liveParamMax(const std::string& organism, const std::string& param);
    void batchLiveParamValues(const std::string& name,
                              const std::vector<hum::Parameter>& params,
                              std::vector<double>& out) const;
    std::string liveParamText(const std::string& organism, const std::string& param);
    void beginParamDrag(const std::string& organism, const std::string& param);
    void endParamDrag();

    FileHost& files() { return files_; }
    DeckHost& decks() { return decks_; }
    MidiHost& midi() { return midi_; }
    const MidiHost& midi() const { return midi_; }
    OscHost& osc() { return osc_; }
    const OscHost& osc() const { return osc_; }
    ModHost& mod() { return mod_; }
    const ModHost& mod() const { return mod_; }
    GamepadHost& gamepads() { return gamepads_; }

    enum MidiSync { kSyncOff = 0, kSyncGenerate = 1, kSyncChase = 2 };
    int  midiSyncMode() const { return syncMode_.load(std::memory_order_relaxed); }
    void setMidiSyncMode(int mode);
    void applyMidiSyncFromSettings();

    void setLinkEnabled(bool on);
    bool linkEnabled() const { return linkEnabled_.load(std::memory_order_relaxed); }
    int  linkPeers() const;
    void applyLinkFromSettings();
    void setLinkStartStopSync(bool on);
    bool linkStartStopSync() const { return linkStartStop_.load(std::memory_order_relaxed); }

    static constexpr int kMidiPorts = 8;
    void pollMidiControl();
    void pollMidiOut();
    void routeLiveMidi(const juce::MidiMessage& m, int port = -1);
    void pushToMonitors(const juce::MidiMessage& m, int port);
    void setLiveTargets(std::vector<std::string> nodes) {
        const juce::ScopedLock ml(midiTargetsLock_);
        liveTargets_ = std::move(nodes);
    }
    std::vector<std::string> liveTargets() const {
        const juce::ScopedLock ml(midiTargetsLock_);
        return liveTargets_;
    }

    void injectLiveMidi(const juce::MidiMessage& m);
    void injectLiveMidiToNode(const std::string& node, const juce::MidiMessage& m);
    void sendMidiOut(const std::string& organism, const std::string& param, double value);
    void flushMidiRecording(bool finalize = false);
    static constexpr int kClipOnDemand = -2;
    void finishOnDemandClips();
    void syncPluginStateToModel();
    HostedPlugin* hostedPluginFor(const std::string& name);
    PluginNode* pluginNodeFor(const std::string& name);
    Organism* liveOrganism(const std::string& name) {
        return graph_ ? graph_->find(name) : nullptr;
    }
    void pollBridges();
    void pumpPluginTunings();
    void pollTuningProbes();
    void restartPluginNode(const std::string& name);
    std::function<void(const std::function<bool(const std::string&)>& survives)>
        onBeforeRebuild;
    std::function<void()> onTopologyChanged;
    std::function<void()> onArrangementChanged;

    std::function<void(const std::string& organism, const std::string& param)>
        openParameterControl;

    std::function<void(const std::string& organism)> onNodeRolled;

    PresetHost& presets() { return presets_; }

    AutomationHost& automation() { return automation_; }
    const AutomationHost& automation() const { return automation_; }

    RecordHost& record() { return record_; }
    void keepLast(int bars);
    const std::string& documentPath() const { return docPath_; }
    bool isLiveTracked(const std::string& organism, const std::string& param) const;
    bool isExternallyControlled(const std::string& organism,
                               const std::string& param) const;

    PatternHost& patterns() { return patterns_; }

    ClipEditor& clips() { return clips_; }

    MetaEditor& metapad() { return metapad_; }

    double songLengthBeats() const { return model_.clock.songLength; }
    void setSongLengthBeats(double beats);
    double songEndBeat() const;
    double songEndSeconds() const {
        const double bpm = tempo() > 0.0 ? tempo() : 120.0;
        return songEndBeat() * 60.0 / bpm;
    }
    void goToEnd() { setPositionBeats(songEndBeat()); }

    void pushUndo();
    void pushParamStep();
    void beginTransaction();
    void endTransaction();
    bool canUndo() const { return !undo_.empty(); }
    bool canRedo() const { return !redo_.empty(); }

    bool isDirty() const { return dirty_; }
    int pendingPatternSyncs() const { return (int) patternPending_.size(); }
    int undoDepth() const { return (int) undo_.size(); }
    bool topUndoIsSnapshot() const { return undo_.empty() || undo_.back().snapshot; }
    int topUndoReverts() const {
        if (undo_.empty()) return 0;
        return (int) (undo_.back().params.size() + undo_.back().patterns.size());
    }
    bool undo();
    bool redo();

    struct LiveControlScope {
        explicit LiveControlScope(EngineHost& h) : host_(h), prev_(h.liveControl_) { h.liveControl_ = true; }
        ~LiveControlScope() { host_.liveControl_ = prev_; }
        EngineHost& host_;
        bool prev_;
    };

    struct PatternSyncBatch {
        explicit PatternSyncBatch(EngineHost& h) : host_(h) { ++host_.patternBatch_; }
        ~PatternSyncBatch() {
            if (--host_.patternBatch_ > 0) return;
            auto pending = std::move(host_.patternPending_);
            host_.patternPending_.clear();
            for (const auto& n : pending) host_.syncPattern(n);
        }
        EngineHost& host_;
    };
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
    void pokeLiveRefresh() { ++liveControlGen_; }

    std::string clockNodeName();
    std::function<std::string()> noteCaptureHint;
    std::string noteCaptureTarget() const;
    std::string audioCaptureTarget() const;
    bool commitRetroactiveAudio(double startBeat, double nowBeat, double beats);

    std::string clockNodeNameIfAny() const {
        for (const auto& cm : model_.organisms)
            if (isClockPseudo(cm.displayClass)) return cm.name;
        return {};
    }

    std::string metapadNodeName();
    std::string metapadNodeNameIfAny() const {
        for (const auto& cm : model_.organisms)
            if (isMetapadPseudo(cm.displayClass)) return cm.name;
        return {};
    }
    double metapadX() const { return metaX_; }
    double metapadY() const { return metaY_; }

    void nodeActivity(const std::string& name, float& audioPeak, float& midiCount);

    int nodeMeter(const std::string& name, float* levels, int maxCh);

    bool audioAlive() const {
        return juce::Time::getMillisecondCounterHiRes()
                   - lastCallbackMs_.load(std::memory_order_relaxed) < 250.0;
    }

    bool armNodeCapture(int samples);
    void disarmNodeCapture();
    int nodeCaptureFill(const std::string& name);
    const float* nodeCaptureData(const std::string& name);

    bool nodeIsNoteTrack(const std::string& name);
    void syncNodeTrack(const std::string& name);

    int inletsOf(const std::string& name);
    int outletsOf(const std::string& name);
    int midiInletsOf(const std::string& name);
    int midiOutletsOf(const std::string& name);
    int videoInletsOf(const std::string& name);
    int videoOutletsOf(const std::string& name);

    std::string masterOutputName();
    int connectToMaster(const std::string& node);
    std::vector<std::string> arrangeableNodes();
    bool nodeRecordsAudio(const std::string& name);

    void startAudioAsync(std::function<void(bool ok, std::string error)> done = {});
    bool ensureAudio();
    void stopAudio();
    bool audioRunning() const { return audioRunning_; }
    bool audioStarting() const { return audioStarting_; }
    juce::AudioDeviceManager& audioDevices() { return devices_; }
    int maxTrackHeldForTest() const { return graph_ ? graph_->maxTrackHeld() : 0; }
    void resetMaxTrackHeldForTest() { if (graph_) graph_->resetMaxTrackHeld(); }
    void startDeviceForTest(juce::AudioIODevice& d) { audioDeviceAboutToStart(&d); }
    void renderOrganismForTest(const std::string& name, float* const* out, int numOut, int n) {
        const juce::ScopedLock sl(lock_);
        if (auto* c = graph_ ? graph_->find(name) : nullptr)
            c->process(nullptr, 0, out, numOut, n, graph_->transport());
    }
    void renderBlockForTest(float* const* out, int numOut, int numSamples) {
        fadeTarget_.store(1.0f);
        audioDeviceIOCallbackWithContext(nullptr, 0, out, numOut, numSamples, {});
    }
    void persistAudioState();

    std::vector<std::pair<int, std::string>> choiceItems(const std::string& source,
                                                         const std::string& organism = {});

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
    double groove() const { return model_.groove; }
    std::string grooveUnit() const { return model_.grooveUnit; }

    void setMetronome(bool on) { metronome_.store(on, std::memory_order_relaxed); }
    bool metronome() const { return metronome_.load(std::memory_order_relaxed); }
    void setMetronomeGain(float g) { metroGain_.store(juce::jlimit(0.0f, 1.0f, g)); }
    float metronomeGain() const { return metroGain_.load(); }
    void setCountIn(bool on) { countInEnabled_.store(on, std::memory_order_relaxed); }
    bool countIn() const { return countInEnabled_.load(std::memory_order_relaxed); }
    bool countInRunning() const { return countInLeft_.load() > 0; }
    void serviceCountIn();

    void pumpOscOut();
    double sampleRate() const { return sampleRate_; }

    void play();
    void playFromStart();
    void stop();
    void goToStart();
    void setPositionBeats(double beat);
    bool isPlaying() const { return playing_; }
    void setTempo(double bpm);
    double tempo() const { return model_.clock.tempo; }
    double liveTempo() const { return graph_ ? graph_->transport().tempo() : model_.clock.tempo; }

    const std::string& notes() const { return model_.notes; }
    void setNotes(const std::string& s) {
        if (model_.notes == s) return;
        model_.notes = s;
        markDirty();
    }
    int positionBar();
    double positionBeat();
    double positionBeats();
    double positionSeconds();
    float outputLevel(int channel) const;

    unsigned dropoutCount() const { return dropouts_.load(std::memory_order_relaxed); }
    float audioLoad() const { return audioLoad_.load(std::memory_order_relaxed); }
    unsigned deviceRestartCount() const { return deviceRestarts_.load(std::memory_order_relaxed); }

    std::string consolidate(const std::string& node, double fromBeat, double toBeat,
                            std::string& error);
    std::string bounceSourceOf(const std::string& node);
    std::string printToTimeline(const std::string& node, std::string& error, int atTick = -1,
                                const std::string& preferredTarget = {});
    std::vector<std::string> noteTargets(const std::string& node);

    bool renderToFile(const std::string& path, double seconds, std::string& error);

    bool startMixRecording(const std::string& path, std::string& error);
    void stopMixRecording();
    bool isMixRecording() const { return mixRecording_.load(); }

    void primeOffline(int blocks);

private:
    friend class MetaEditor;
    MetaEditor metapad_{*this};
    friend class ClipEditor;
    ClipEditor clips_{*this};
    friend class DeckHost;
    DeckHost decks_{*this};
    friend class PatternHost;
    PatternHost patterns_{*this};
    friend class PresetHost;
    PresetHost presets_{*this};
    friend class FileHost;
    FileHost files_{*this};
    friend class AutomationHost;
    AutomationHost automation_{*this};
    friend class MidiHost;
    MidiHost midi_{*this};
    OscHost osc_{*this};
    ModHost mod_{*this};
    GamepadHost gamepads_{*this};
    friend class RecordHost;
    RecordHost record_{*this};
    std::string docPath_;

    void onFileNodeChanged(const std::string& organism, const std::string& filePath,
                           bool sourceMoved = true);
    void autoDetectDeckGrid(const std::string& organism, const std::string& filePath);
    void applyGroove();
    void rebuild();
    void applyMidiTrackTarget(const std::string& name, int value);
    void syncMidiTrackTargets();
    void discardLiveGraph();
    void publishClock();
    void applyLoadedLayout();
    void syncViewsFromPositions();
    static int countInlets(const PatchDocumentModel&, const std::string& name);
    static int countOutlets(const PatchDocumentModel&, const std::string& name);
    void dropInvalidConnections();
    void syncAutomation();
    void syncPattern(const std::string& name);
    void markPatternEdited() { if (!liveControl_) dirty_ = true; }
    void renameReferences(const std::string& oldName, const std::string& newName);
    void promotePodStrays(const std::string& pod, int domain,
                          juce::Point<int> topLeft, juce::Point<int> bottomLeft);
    void splicePodPorts(const std::string& pod, int domain);
    OrganismModel* mutableByName(const std::string& name);
    void beginCapturePass() {
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
    void clearTouches();
    struct CapturePass { double firstBeat, lastBeat; double lo = 0.0, hi = 0.0; bool isRange = false; };
    bool latch_ = false;
    std::map<std::pair<std::string, std::string>, CapturePass> capturePass_;
    void endCapturePasses();
    bool capturing_ = false;
public:
    void extendLatchPasses();
    void setLatchMode(bool on) { latch_ = on; }
    bool latchMode() const { return latch_; }
private:
    unsigned laneStamp_ = 0;

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
    juce::CriticalSection midiTargetsLock_;
    struct MidiTarget { PluginNode* hp; int mode; int channel; };
    std::vector<MidiTarget> midiTargets_;
    std::vector<LiveMidiIn*> liveMidiIns_;
    std::vector<PendingMidiOut*> midiDrains_;
    std::vector<LiveMidiIn*> midiMonitors_;
    std::vector<std::pair<std::string, LiveMidiIn*>> namedLiveIns_;
    struct MidiRecTarget {
        std::string node; int clip = -1; int quantize = 0; bool thru = false; bool grow = false;
    };
    std::vector<MidiRecTarget> midiRecordTargets_;
    bool midiRecordUndoPushed_ = false;
    std::vector<std::string> liveTargets_;
    std::set<std::string> solo_;
    std::vector<std::string> soloable_;
    std::vector<TimedMidi> midiCapture_;
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
    void recordPatternRevert(const std::string& organism);
    UndoState inverseOf(const UndoState& step) const;
    void applyDelta(const UndoState& step);
    bool inTxn_ = false;
    bool txnPushed_ = false;
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

    double sampleRate_ = 44100.0;
    int block_ = 512;
    bool monoFold_ = false;
    std::atomic<int> refusedBlocks_{0};
    std::atomic<int> refusedLargest_{0};
    int outputLatency_ = 0;
    double preparedSampleRate_ = 0.0;
    int preparedBlock_ = 0;

    juce::AudioDeviceManager devices_;
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

private:
    static inline bool ignoreSavedFormat_ = false;
    static inline bool headless_ = false;
    bool auxCheckPending_ = false;
    std::shared_ptr<bool> hostAlive_ = std::make_shared<bool>(true);
    std::atomic<bool> playing_{false};
    bool masterRecord_ = false;
    std::atomic<float> level_[2] {0.0f, 0.0f};
    std::atomic<double> lastCallbackMs_ {0.0};

    std::vector<std::unique_ptr<juce::MidiInput>> midiInputs_;
    std::vector<int> midiInputPorts_;
    std::unique_ptr<juce::MidiOutput> midiOuts_[kMidiPorts];
    juce::MidiOutput* midiOutForPort(int p) {
        return p >= 0 && p < kMidiPorts ? midiOuts_[p].get() : nullptr;
    }
    bool anyMidiOutOpen() const {
        for (const auto& o : midiOuts_) if (o) return true;
        return false;
    }
    juce::MidiDeviceListConnection midiDeviceListConn_;
    MidiControlMap midiMap_;
    bool midiEnabled_ = false;
    std::atomic<int> ccValue_[kMidiSourceCount];
    std::atomic<int> lastCc_ {-1};
    int ccLastApplied_[kMidiSourceCount];
    double ctrlPollMs_ = 0.0;
    double modPollMs_ = 0.0;
    double captureStartBeat_ = 0.0;
    double metaX_ = 0.5, metaY_ = 0.5;
    bool metaSnapArmed_ = false;
    bool metaHoldApply_ = false;
    int metaLastRecall_ = -1;
    double metaRecallBeat_ = 0.0;
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
    std::atomic<bool> countInEnabled_ {false};
    std::atomic<std::int64_t> countInLeft_ {0};
    std::atomic<bool> countInFire_ {false};
    std::int64_t countInTotal_ = 0;
    double countInNextBeat_ = 0.0;
    float fadeGain_ = 0.0f;
};

}
