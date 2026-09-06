#include "gui/EngineHost.h"

#include "core/BankLibrary.h"
#include "core/UserLibrary.h"

#include <algorithm>
#include <cstddef>
#include <map>
#include <utility>
#include <vector>

#include "core/GraphLayout.h"
#include "core/HostedPlugin.h"
#include "core/PluginNode.h"
#include "core/PackRegistry.h"
#include "core/PerfBox.h"
#include "core/PodModel.h"
#include "hum/Registry.h"
#include "io/PatchWriter.h"
#include "io/PatchLoader.h"
#include "gui/AppSettings.h"

namespace hum {

EngineHost::EngineHost() {
    for (int i = 0; i < kMidiSourceCount; ++i) { ccValue_[i].store(-1); ccLastApplied_[i] = -1; }
    for (int i = 0; i < kMaxDeviceChannels; ++i) { inMap_[i] = i; outMap_[i] = i; }
    seedDeviceFormatFromSettings();
    midiDeviceListConn_ = juce::MidiDeviceListConnection::make([this] {
        if (midiEnabled_) midi().refreshDevices();
    });
    if (AppSettings::instance().getInt("osc.enabled", 0) != 0) osc_.setEnabled(true);
    if (const auto lib = AppSettings::instance().getString("library.path"); lib.isNotEmpty())
        library::configuredRoot() = lib.toStdString();
    latch_ = AppSettings::instance().getInt("automation.latch", 0) != 0;
    registerBuiltinOrganisms();
    newDocument();
}

EngineHost::~EngineHost() {
    *hostAlive_ = false;
    midi().setEnabled(false);
    stopAudio();
}

void EngineHost::newDocument(juce::Point<int> masterPos) {
    stopAudio();
    discardLiveGraph();
    beginTransaction();
    pushUndo();
    model_ = PatchDocumentModel{};
    model_.version = "1";
    model_.clock.tempo = 120.0;
    outputGain_.store(1.0f);
    limiterOn_.store(false);
    docPath_.clear();
    positions_.clear();
    original_.reset();
    if (PackRegistry::instance().isClassEnabled("SoundOut"))
        addOrganism("SoundOut", masterPos);
    endTransaction();
    undo_.clear();
    redo_.clear();
    paramHistory_.clear();
    dirty_ = false;
}

std::size_t EngineHost::approxBytes(const UndoState& s) {
    auto params = [](const std::vector<Parameter>& v) {
        std::size_t n = v.capacity() * sizeof(Parameter);
        for (const auto& p : v) n += p.name.size() + p.type.size() + p.text.size();
        return n;
    };
    auto pattern = [](const Pattern& p) {
        std::size_t n = p.channels.capacity() * sizeof(PatternChannel);
        for (const auto& ch : p.channels)
            n += ch.matrix.size() + ch.name.size() + ch.audioFile.size() + ch.snap.size()
                 + ch.triggers.capacity() * sizeof(int)
                 + ch.timeSignatures.capacity() * sizeof(PatternTimeSig);
        return n;
    };
    std::size_t n = sizeof(UndoState);
    if (s.snapshot) {
        n += s.model.organisms.capacity() * sizeof(OrganismModel);
        for (const auto& c : s.model.organisms) {
            n += c.name.size() + c.classRaw.size() + c.displayClass.size() + c.kind.size();
            n += params(c.properties) + pattern(c.pattern);
            for (const auto& pr : c.presets) n += pr.name.size() + params(pr.properties);
            for (const auto& l : c.automation)
                n += l.propertyName.size()
                     + l.points.capacity() * sizeof(AutomationBreakpoint);
            n += c.midiSources.capacity() * sizeof(MidiControllerSource)
                 + c.oscSources.capacity() * sizeof(OscControllerSource)
                 + c.modSources.capacity() * sizeof(ModControllerSource);
        }
        n += s.model.connections.capacity() * sizeof(ConnectionModel) * 3;
        n += s.model.views.capacity() * sizeof(OrganismView);
        n += s.positions.size() * (sizeof(juce::Point<int>) + 32);
    } else {
        for (const auto& r : s.params) n += sizeof(ParamRevert) + r.organism.size()
                                            + r.param.size() + r.before.text.size();
        for (const auto& r : s.patterns) n += sizeof(PatternRevert) + r.organism.size()
                                             + pattern(r.before);
    }
    return n;
}

void EngineHost::trimHistory(std::deque<UndoState>& d) {
    while (d.size() > kUndoMaxSteps) d.pop_front();
    std::size_t total = 0;
    for (const auto& s : d) total += approxBytes(s);
    while (d.size() > 1 && total > kUndoMaxBytes) {
        total -= approxBytes(d.front());
        d.pop_front();
    }
}

void EngineHost::pushUndo() {
    const bool wasDirty = dirty_;
    dirty_ = true;
    ++changeStamp_;
    paramEditKey_.clear();
    paramStepOpen_ = false;
    if (inTxn_ && txnPushed_) return;
    UndoState s;
    s.snapshot = true;
    s.wasDirty = wasDirty;
    s.model = model_;
    s.positions = positions_;
    undo_.push_back(std::move(s));
    trimHistory(undo_);
    redo_.clear();
    if (inTxn_) txnPushed_ = true;
}

void EngineHost::pushParamStep() {
    bool wasDirty = dirty_;
    dirty_ = true;
    ++changeStamp_;
    if (!undo_.empty() && !undo_.back().snapshot && undo_.back().params.empty()
        && undo_.back().patterns.empty()) {
        wasDirty = undo_.back().wasDirty;
        undo_.pop_back();
    }
    UndoState s;
    s.snapshot = false;
    s.wasDirty = wasDirty;
    undo_.push_back(std::move(s));
    trimHistory(undo_);
    redo_.clear();
    paramStepOpen_ = true;
}

void EngineHost::recordParamRevert(const std::string& organism, const std::string& param) {
    if (!paramStepOpen_ || undo_.empty() || undo_.back().snapshot) return;
    auto& step = undo_.back();
    for (const auto& r : step.params)
        if (r.organism == organism && r.param == param) return;
    ParamRevert r;
    r.organism = organism;
    r.param = param;
    r.existed = false;
    if (const auto* cm = model_.byName(organism))
        for (const auto& p : cm->properties)
            if (p.name == param) { r.before = p; r.existed = true; break; }
    if (!r.existed) { r.before.name = param; }
    step.params.push_back(std::move(r));
}

void EngineHost::recordPatternRevert(const std::string& organism) {
    if (!paramStepOpen_ || undo_.empty() || undo_.back().snapshot) return;
    auto& step = undo_.back();
    for (const auto& r : step.patterns)
        if (r.organism == organism) return;
    const auto* cm = model_.byName(organism);
    if (cm == nullptr) return;
    step.patterns.push_back({organism, cm->pattern});
}

EngineHost::UndoState EngineHost::inverseOf(const UndoState& step) const {
    UndoState inv;
    inv.snapshot = false;
    for (const auto& r : step.params) {
        ParamRevert back;
        back.organism = r.organism;
        back.param = r.param;
        back.existed = false;
        if (const auto* cm = model_.byName(r.organism))
            for (const auto& p : cm->properties)
                if (p.name == r.param) { back.before = p; back.existed = true; break; }
        if (!back.existed) back.before.name = r.param;
        inv.params.push_back(std::move(back));
    }
    for (const auto& r : step.patterns)
        if (const auto* cm = model_.byName(r.organism))
            inv.patterns.push_back({r.organism, cm->pattern});
    return inv;
}

void EngineHost::applyDelta(const UndoState& step) {
    const bool wasOpen = paramStepOpen_;
    paramStepOpen_ = false;
    DerivedControlScope derived(*this);
    {
        PatternSyncBatch batch(*this);
        for (const auto& r : step.patterns)
            if (auto* cm = mutableByName(r.organism)) {
                cm->pattern = r.before;
                syncPattern(r.organism);
            }
    }
    for (auto it = step.params.rbegin(); it != step.params.rend(); ++it) {
        const auto& r = *it;
        auto* cm = mutableByName(r.organism);
        if (cm == nullptr) continue;
        if (!r.existed) {
            auto& v = cm->properties;
            v.erase(std::remove_if(v.begin(), v.end(),
                                   [&](const Parameter& p) { return p.name == r.param; }),
                    v.end());
            continue;
        }
        const std::string* curText = nullptr;
        for (const auto& p : cm->properties)
            if (p.name == r.param) { curText = &p.text; break; }
        if (curText != nullptr && *curText != r.before.text)
            setParamText(r.organism, r.param, r.before.text);
        if (r.before.isRange) setParamRange(r.organism, r.param, r.before.rangeMin, r.before.rangeMax);
        else                  setParam(r.organism, r.param, r.before.value);
    }
    paramStepOpen_ = wasOpen;
}

void EngineHost::beginTransaction() { inTxn_ = true; txnPushed_ = false; }
void EngineHost::endTransaction() { inTxn_ = false; txnPushed_ = false; }

bool EngineHost::undo() {
    if (undo_.empty()) return false;
    const bool wasDirty = dirty_;
    paramEditKey_.clear();
    paramStepOpen_ = false;
    auto s = std::move(undo_.back());
    undo_.pop_back();
    if (!s.snapshot) {
        auto inv = inverseOf(s);
        inv.wasDirty = wasDirty;
        redo_.push_back(std::move(inv));
    trimHistory(redo_);
        applyDelta(s);
        dirty_ = s.wasDirty;
        return true;
    }
    UndoState cur;
    cur.snapshot = true;
    cur.wasDirty = wasDirty;
    cur.model = model_;
    cur.positions = positions_;
    redo_.push_back(std::move(cur));
    trimHistory(redo_);
    model_ = std::move(s.model);
    positions_ = std::move(s.positions);
    modelSwapped_ = true;
    rebuild();
    dirty_ = s.wasDirty;
    return true;
}

bool EngineHost::redo() {
    if (redo_.empty()) return false;
    const bool wasDirty = dirty_;
    paramEditKey_.clear();
    paramStepOpen_ = false;
    auto s = std::move(redo_.back());
    redo_.pop_back();
    if (!s.snapshot) {
        auto inv = inverseOf(s);
        inv.wasDirty = wasDirty;
        undo_.push_back(std::move(inv));
        applyDelta(s);
        dirty_ = s.wasDirty;
        return true;
    }
    UndoState cur;
    cur.snapshot = true;
    cur.wasDirty = wasDirty;
    cur.model = model_;
    cur.positions = positions_;
    undo_.push_back(std::move(cur));
    model_ = std::move(s.model);
    positions_ = std::move(s.positions);
    modelSwapped_ = true;
    rebuild();
    dirty_ = s.wasDirty;
    return true;
}

juce::Point<int> EngineHost::position(const std::string& name) const {
    auto it = positions_.find(name);
    return it == positions_.end() ? juce::Point<int>{40, 40} : it->second;
}

juce::Point<int> EngineHost::editorPosition(const std::string& name) const {
    for (auto& v : model_.views)
        if (v.organismName == name && v.hasEditor) return {v.editorX, v.editorY};
    return {-1, -1};
}

bool EngineHost::editorVisible(const std::string& name) const {
    for (auto& v : model_.views)
        if (v.organismName == name) return v.hasEditor && v.editorVisible;
    return false;
}

void EngineHost::setEditorState(const std::string& name, juce::Point<int> pos, bool visible) {
    for (auto& v : model_.views)
        if (v.organismName == name) {
            v.hasEditor = true; v.editorVisible = visible; v.editorX = pos.x; v.editorY = pos.y;
            return;
        }
    OrganismView v;
    v.organismName = name;
    v.hasEditor = true; v.editorVisible = visible; v.editorX = pos.x; v.editorY = pos.y;
    model_.views.push_back(v);
}

juce::Point<int> EngineHost::editorSize(const std::string& name) const {
    for (auto& v : model_.views)
        if (v.organismName == name) return {v.editorW, v.editorH};
    return {0, 0};
}

int EngineHost::editorHalf(const std::string& name) const {
    for (auto& v : model_.views)
        if (v.organismName == name) return v.editorHalf;
    return -1;
}

void EngineHost::setEditorSize(const std::string& name, juce::Point<int> size, int half) {
    dirty_ = true;
    for (auto& v : model_.views)
        if (v.organismName == name) {
            v.editorW = size.x; v.editorH = size.y; v.editorHalf = half;
            return;
        }
    OrganismView v;
    v.organismName = name;
    v.editorW = size.x; v.editorH = size.y; v.editorHalf = half;
    model_.views.push_back(v);
}

bool EngineHost::editorCollapsed(const std::string& name) const {
    for (auto& v : model_.views)
        if (v.organismName == name) return v.editorCollapsed;
    return false;
}

void EngineHost::setEditorCollapsed(const std::string& name, bool collapsed) {
    dirty_ = true;
    for (auto& v : model_.views)
        if (v.organismName == name) { v.editorCollapsed = collapsed; return; }
    OrganismView v;
    v.organismName = name;
    v.editorCollapsed = collapsed;
    model_.views.push_back(v);
}

int EngineHost::editorMode(const std::string& name) const {
    for (auto& v : model_.views)
        if (v.organismName == name) return v.editorMode;
    return -1;
}

void EngineHost::setEditorMode(const std::string& name, int mode) {
    dirty_ = true;
    for (auto& v : model_.views)
        if (v.organismName == name) { v.editorMode = mode; return; }
    OrganismView v;
    v.organismName = name;
    v.editorMode = mode;
    model_.views.push_back(v);
}

int EngineHost::countInlets(const PatchDocumentModel& m, const std::string& name) {
    int n = 0;
    for (auto& c : m.connections) if (c.dst == name) n = std::max(n, c.dstInlet + 1);
    return n;
}
int EngineHost::countOutlets(const PatchDocumentModel& m, const std::string& name) {
    int n = 0;
    for (auto& c : m.connections) if (c.src == name) n = std::max(n, c.srcOutlet + 1);
    return n;
}

int EngineHost::inletsOf(const std::string& name) {
    if (graph_) if (auto* c = graph_->find(name)) return std::max(c->numAudioInputs(), countInlets(model_, name));
    return countInlets(model_, name);
}
int EngineHost::outletsOf(const std::string& name) {
    if (graph_) if (auto* c = graph_->find(name)) return std::max(c->numAudioOutputs(), countOutlets(model_, name));
    return countOutlets(model_, name);
}

void EngineHost::nodeActivity(const std::string& name, float& audioPeak, float& midiCount) {
    audioPeak = 0.0f;
    midiCount = 0.0f;
    if (!graph_) return;
    const int i = graph_->indexOf(name);
    if (i < 0) return;
    audioPeak = graph_->nodeAudioActivity(i);
    midiCount = graph_->nodeMidiActivity(i);
}

bool EngineHost::armNodeCapture(int samples) {
    const juce::ScopedLock sl(lock_);
    if (!graph_) return false;
    graph_->armCapture(samples);
    return true;
}

void EngineHost::disarmNodeCapture() {
    const juce::ScopedLock sl(lock_);
    if (graph_) graph_->disarmCapture();
}

int EngineHost::nodeCaptureFill(const std::string& name) {
    return graph_ ? graph_->captureFill(graph_->indexOf(name)) : 0;
}

const float* EngineHost::nodeCaptureData(const std::string& name) {
    return graph_ ? graph_->captureData(graph_->indexOf(name)) : nullptr;
}

int EngineHost::nodeMeter(const std::string& name, float* levels, int maxCh) {
    auto* src = dynamic_cast<LevelMeterSource*>(liveOrganism(name));
    if (src == nullptr) return 0;
    const int n = std::min(src->meterChannels(), maxCh);
    const bool live = audioAlive() && !bypassed(name);
    for (int c = 0; c < n; ++c) levels[c] = live ? src->meterLevel(c) : 0.0f;
    return n;
}

std::string EngineHost::masterOutputName() {
    if (graph_)
        for (int i = 0; i < graph_->nodeCount(); ++i)
            if (auto* c = graph_->organism(i))
                if (dynamic_cast<MasterTap*>(c)) return c->name();
    return {};
}

juce::Point<int> EngineHost::freeSpot(juce::Point<int> want) const {
    constexpr int kW = 150, kH = 64;
    auto taken = [&](juce::Point<int> at) {
        for (const auto& cm : model_.organisms) {
            const auto p = position(cm.name);
            if (std::abs(p.x - at.x) < kW && std::abs(p.y - at.y) < kH) return true;
        }
        return false;
    };
    for (int col = 0; col < 12; ++col)
        for (int rowN = 0; rowN < 24; ++rowN) {
            const juce::Point<int> at{want.x + col * kW, want.y + rowN * kH};
            if (!taken(at)) return at;
        }
    return want;
}

juce::Point<int> EngineHost::spotBelowPatch() const {
    const auto master = const_cast<EngineHost*>(this)->masterOutputName();
    int bottom = 60, left = 60, masterY = 0;
    bool any = false, haveMaster = false;
    for (const auto& cm : model_.organisms) {
        const auto p = position(cm.name);
        if (!master.empty() && cm.name == master) {
            masterY = p.y;
            haveMaster = true;
            continue;
        }
        if (!any) { left = p.x; any = true; }
        left = std::min(left, p.x);
        bottom = std::max(bottom, p.y);
    }
    int y = any ? bottom + 80 : 140;
    if (haveMaster) y = std::min(y, masterY - 80);
    return freeSpot({left, std::max(60, y)});
}

std::vector<std::string> EngineHost::arrangeableNodes() {
    std::vector<std::string> out;
    if (graph_ == nullptr) return out;
    for (int i = 0; i < graph_->nodeCount(); ++i) {
        auto* c = graph_->organism(i);
        if (c == nullptr) continue;
        if (nodeIsInternal(c->name())) continue;
        if (dynamic_cast<ClipRecorder*>(c) != nullptr || nodeIsNoteTrack(c->name())
            || dynamic_cast<VideoTimelineSource*>(c) != nullptr)
            out.push_back(c->name());
    }
    return out;
}

bool EngineHost::nodeRecordsAudio(const std::string& name) {
    if (!graph_) return false;
    return dynamic_cast<ClipRecorder*>(graph_->find(name)) != nullptr;
}

bool EngineHost::nodeArrangesVideo(const std::string& name) {
    if (!graph_) return false;
    return dynamic_cast<VideoTimelineSource*>(graph_->find(name)) != nullptr;
}

bool EngineHost::nodeRecordsVideo(const std::string& name) {
    if (!graph_) return false;
    auto* live = graph_->find(name);
    auto* vn = dynamic_cast<VideoNode*>(live);
    return dynamic_cast<VideoTimelineSource*>(live) != nullptr && vn != nullptr
           && vn->numVideoInputs() > 0;
}

void EngineHost::applyGroove() {
    if (graph_ == nullptr) return;
    graph_->transport().setGroove(swing::grooveFor(model_.groove, model_.grooveUnit));
}

void EngineHost::rebuild() {
    ++laneStamp_;
    syncMidiTrackTargets();

    outputGain_.store((float) juce::jlimit(0.0, 1.0, model_.masterLevel));
    limiterOn_.store(model_.masterLimiter);
    applyGroove();
    const bool canAdopt =
        graph_ != nullptr && sampleRate_ == preparedSampleRate_ && block_ == preparedBlock_;
    if (onBeforeRebuild)
        onBeforeRebuild([&](const std::string& name) {
            if (!canAdopt) return false;
            const auto* cm = model_.byName(name);
            if (cm == nullptr) return false;
            const Organism* live = graph_->find(name);
            return live != nullptr && !live->matchToken().empty()
                   && live->matchToken() == cm->classRaw;
        });
    {
        const juce::ScopedLock ml(midiTargetsLock_);
        midiTargets_.clear();
        liveMidiIns_.clear();
        midiMonitors_.clear();
        namedLiveIns_.clear();
        midiDrains_.clear();
    }

    std::function<const Organism*(const OrganismModel&)> reuse;
    if (canAdopt)
        reuse = [this](const OrganismModel& cm) -> const Organism* {
            const Organism* live = graph_->find(cm.name);
            return live != nullptr && !live->matchToken().empty()
                        && live->matchToken() == cm.classRaw ? live : nullptr;
        };
    auto g = std::make_unique<AudioGraph>();
    std::string err;
    if (!buildGraph(model_, *g, err, reuse)) return;
    if (canAdopt) g->prepare(sampleRate_, block_, model_.clock.tempo, *graph_);
    else g->prepare(sampleRate_, block_, model_.clock.tempo);

    const bool audible = audioRunning_ && fadeGainPub_.load() > 0.0005f;
    if (audible) {
        fadeTarget_.store(0.0f);
        for (int i = 0; i < 60 && fadeGainPub_.load() > 0.0005f; ++i)
            juce::Thread::sleep(1);
    }

    std::unique_ptr<AudioGraph> retired;
    {
        const juce::ScopedLock sl(lock_);
        const int64_t pos = graph_ ? graph_->transport().samplePosition() : 0;
        const double beats = graph_ ? graph_->transport().beats() : 0.0;
        g->transport().setPlaying(playing_);
        g->transport().restorePosition(pos, beats);
        g->setExternalTempoMaster(linkEnabled_.load(std::memory_order_relaxed));
        if (canAdopt) g->adoptMatchingNodes(*graph_);
        std::vector<std::pair<MasterTap*, int>> taps;
        std::vector<std::pair<HardwareOut*, int>> auxes;
        for (int i = 0; i < g->nodeCount(); ++i) {
            if (auto* s = dynamic_cast<MasterTap*>(g->organism(i))) taps.emplace_back(s, i);
            else if (auto* a = dynamic_cast<HardwareOut*>(g->organism(i))) auxes.emplace_back(a, i);
        }
        retired = std::move(graph_);
        graph_ = std::move(g);
        soundOut_ = taps.empty() ? nullptr : taps.front().first;
        masterTaps_ = std::move(taps);
        auxOuts_ = std::move(auxes);
        recorders_.clear();
        for (auto& c : model_.organisms)
            if (auto* fr = dynamic_cast<Recorder*>(graph_->find(c.name)))
                recorders_.emplace_back(c.name, fr);
        {
            const juce::ScopedLock ml(midiTargetsLock_);
            for (auto& c : model_.organisms) {
                auto* node = graph_->find(c.name);
                if (!node) continue;
                if (auto* hp = dynamic_cast<PluginNode*>(node))
                    midiTargets_.push_back({hp, c.midiReceiveMode, c.midiReceiveChannel});
                if (auto* in = dynamic_cast<LiveMidiIn*>(node)) {
                    if (in->monitorsAllPorts()) midiMonitors_.push_back(in);
                    else if (in->liveMidiPort() >= 0) liveMidiIns_.push_back(in);
                    namedLiveIns_.push_back({c.name, in});
                }
                if (auto* out = dynamic_cast<PendingMidiOut*>(node))
                    midiDrains_.push_back(out);
            }
        }
        publishClock();
    }
    retired.reset();

    preparedSampleRate_ = sampleRate_;
    preparedBlock_ = block_;
    if (modelSwapped_) {
        modelSwapped_ = false;
        std::vector<Organism*> clipNodes;
        {
            const juce::ScopedLock sl(lock_);
            for (const auto& cm : model_.organisms)
                if (auto* live = graph_ ? graph_->find(cm.name) : nullptr) {
                    live->loadFrom(OrganismState{cm.properties, cm.pattern});
                    clipNodes.push_back(live);
                }
        }
        for (auto* live : clipNodes) {
            if (auto* cr = dynamic_cast<ClipRecorder*>(live)) cr->ensureClipsLoaded();
            if (auto* sa = dynamic_cast<SessionAudio*>(live)) sa->loadSessionAudio();
        }
    }
    syncAutomation();
    for (const auto& cm : model_.organisms) syncNodeTrack(cm.name);
    applySolo();
    for (const auto& cm : model_.organisms)
        for (const auto& p : cm.properties) {
            if (p.name.rfind("File", 0) != 0) continue;
            if (p.text.rfind(kAssetScheme, 0) != 0
                && p.text.rfind(banks::kLegacyPrefix, 0) != 0) continue;
            if (auto* fl = dynamic_cast<FileLoader*>(graph_ ? graph_->find(cm.name) : nullptr))
                fl->loadFromFile(banks::resolve(p.text, cm.displayClass));
        }
    if (audioRunning_) requestFadeIn();
    scheduleAuxChannelCheck();
}

void EngineHost::discardLiveGraph() {
    if (onBeforeRebuild) onBeforeRebuild([](const std::string&) { return false; });
    {
        const juce::ScopedLock ml(midiTargetsLock_);
        midiTargets_.clear();
        liveMidiIns_.clear();
        midiMonitors_.clear();
        namedLiveIns_.clear();
        midiDrains_.clear();
    }
    std::unique_ptr<AudioGraph> retired;
    {
        const juce::ScopedLock sl(lock_);
        retired = std::move(graph_);
        soundOut_ = nullptr;
        masterTaps_.clear();
        auxOuts_.clear();
        recorders_.clear();
        publishClock();
    }
    retired.reset();
}

bool EngineHost::loadFile(const std::string& path, std::string& error) {
    stopAudio();
    PatchDocumentModel m;
    std::unique_ptr<juce::XmlElement> raw;
    if (!parsePatchFile(path, m, error, &raw)) return false;
    discardLiveGraph();
    model_ = std::move(m);
    reconcilePropertyTypes(model_);
    pods::resolvePodVideoCords(model_);
    original_ = std::move(raw);
    docPath_ = path;
    positions_.clear();
    midi().syncMapFromModel();
    osc().syncMapFromModel();
    mod().syncMapFromModel();
    perfbox::derive(model_);
    applyLoadedLayout();
    rebuild();
    undo_.clear();
    redo_.clear();
    paramHistory_.clear();
    dirty_ = false;
    if (link_ && linkEnabled_.load(std::memory_order_relaxed) && link_->numPeers() == 0)
        link_->proposeTempo(model_.clock.tempo);
    return true;
}

void EngineHost::applyLoadedLayout() {
    if (model_.views.empty() && !model_.organisms.empty()) {
        std::vector<LayoutNode> nodes;
        std::map<std::string, int> idx;
        for (auto& c : model_.organisms) {
            idx[c.name] = (int) nodes.size();
            const int ports = std::max(countInlets(model_, c.name), countOutlets(model_, c.name));
            nodes.push_back({std::max(116, 18 + ports * 12), 38});
        }
        std::vector<LayoutEdge> edges;
        auto add = [&](const std::vector<ConnectionModel>& cords) {
            for (auto& cn : cords) {
                const auto s = idx.find(cn.src), d = idx.find(cn.dst);
                if (s != idx.end() && d != idx.end() && s->second != d->second)
                    edges.push_back({s->second, d->second});
            }
        };
        add(model_.connections);
        add(model_.midiConnections);
        add(model_.videoConnections);
        const auto placed = layoutFlowGraph(nodes, edges);
        for (auto& c : model_.organisms) {
            const auto& p = placed[(size_t) idx[c.name]];
            positions_[c.name] = {p.first, p.second};
        }
        return;
    }

    std::map<std::string, const OrganismView*> byName;
    for (auto& v : model_.views) byName[v.organismName] = &v;

    constexpr int margin = 40;
    bool any = false;
    int minX = 0, minY = 0;
    for (auto& v : model_.views) {
        if (!any) { minX = v.patcherX; minY = v.patcherY; any = true; }
        else { minX = std::min(minX, v.patcherX); minY = std::min(minY, v.patcherY); }
    }
    const int offX = any ? margin - minX : 0;
    const int offY = any ? margin - minY : 0;

    int gridIdx = 0;
    for (auto& c : model_.organisms) {
        auto it = byName.find(c.name);
        if (it != byName.end()) {
            positions_[c.name] = {it->second->patcherX + offX, it->second->patcherY + offY};
        } else {
            positions_[c.name] = {60 + (gridIdx % 4) * 180, 60 + (gridIdx / 4) * 130};
            ++gridIdx;
        }
    }

    for (auto& v : model_.views)
        if (!model_.byName(v.organismName) && pods::isPod(model_, v.organismName))
            positions_[v.organismName] = {v.patcherX + offX, v.patcherY + offY};
}

void EngineHost::syncViewsFromPositions() {
    std::vector<OrganismView> merged;
    merged.reserve(model_.organisms.size());
    for (auto& c : model_.organisms) {
        OrganismView v;
        v.organismName = c.name;
        auto p = position(c.name);
        v.patcherX = p.x;
        v.patcherY = p.y;
        for (auto& old : model_.views)
            if (old.organismName == c.name) {
                v.hasEditor = old.hasEditor;
                v.editorVisible = old.editorVisible;
                v.editorX = old.editorX;
                v.editorY = old.editorY;
                v.editorMode = old.editorMode;
                v.editorW = old.editorW;
                v.editorH = old.editorH;
                v.editorHalf = old.editorHalf;
                v.editorCollapsed = old.editorCollapsed;
                break;
            }
        merged.push_back(std::move(v));
    }
    for (auto& [n, p] : positions_) {
        if (model_.byName(n) || !pods::isPod(model_, n)) continue;
        OrganismView v;
        v.organismName = n;
        v.patcherX = p.x;
        v.patcherY = p.y;
        merged.push_back(std::move(v));
    }
    model_.views = std::move(merged);
}

bool EngineHost::writeDocumentTo(const std::string& path, std::string& error,
                                 bool pullPluginState) {
    syncViewsFromPositions();
    midi().syncMapToModel();
    osc().syncMapToModel();
    mod().syncMapToModel();
    if (pullPluginState) syncPluginStateToModel();
    storeSessionAudioTo(path);
    return writePatchFile(path, model_, error, original_.get());
}

void EngineHost::storeSessionAudioTo(const std::string& path) {
    const juce::File doc(juce::String(juce::CharPointer_UTF8(path.c_str())));
    const auto dir = doc.getParentDirectory()
                        .getChildFile(doc.getFileNameWithoutExtension() + " Loops");
    for (auto& cm : model_.organisms) {
        auto* sa = dynamic_cast<SessionAudio*>(liveOrganism(cm.name));
        if (sa == nullptr) continue;
        const auto prefix = dir.getChildFile(juce::File::createLegalFileName(
            juce::String(juce::CharPointer_UTF8(cm.name.c_str()))));
        std::vector<std::pair<std::string, std::string>> vals;
        if (!sa->storeSessionAudio(prefix.getFullPathName().toStdString(), vals)) continue;
        for (const auto& [param, text] : vals) {
            Parameter* slot = nullptr;
            for (auto& p : cm.properties)
                if (p.name == param) slot = &p;
            if (slot == nullptr) {
                Parameter np;
                np.index = (int) cm.properties.size();
                np.name = param;
                np.type = "soundfile";
                cm.properties.push_back(np);
                slot = &cm.properties.back();
            }
            if (text.empty() && !slot->text.empty()) {
                const juce::File old(juce::String(juce::CharPointer_UTF8(slot->text.c_str())));
                if (old.isAChildOf(dir)) old.deleteFile();
            }
            slot->text = text;
            slot->userEdited = true;
            const juce::ScopedLock sl(lock_);
            if (graph_)
                if (auto* o = graph_->find(cm.name)) {
                    if (auto* pp = o->params.byName(param)) pp->text = text;
                    else {
                        Parameter np;
                        np.index = (int) o->params.all().size();
                        np.name = param;
                        np.text = text;
                        o->params.add(np);
                    }
                }
        }
    }
}

bool EngineHost::saveFile(const std::string& path, std::string& error) {
    if (!writeDocumentTo(path, error)) return false;
    dirty_ = false;
    for (auto& s : undo_) s.wasDirty = true;
    for (auto& s : redo_) s.wasDirty = true;
    docPath_ = path;
    return true;
}

bool EngineHost::saveCopy(const std::string& path, std::string& error) {
    const bool audible = audioRunning_ && playing_;
    return writeDocumentTo(path, error, !audible);
}

}

