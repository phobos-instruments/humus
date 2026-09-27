// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "Helix/HelixStretch.h"
#include "hum/caps/Files.h"
#include "hum/caps/Graph.h"
#include "hum/Organism.h"
#include "hum/ParamRef.h"

namespace hum {

class Helix : public Organism, public StrandStatus, public StrandIntent, public StrandWave,
              public SessionAudio, public AudioTakes, public WantsNullInlets {
public:
    int numAudioInputs() const override { return 2 + kStrands * 2; }
    int numAudioOutputs() const override { return 2 + kStrands * 2; }

    void prepare(double sampleRate, int maxBlock) override;
    void reset() override;
    void process(const float* const* in, int numIn,
                 float* const* out, int numOut,
                 int numSamples, const Transport& transport) override;

    int strandCount() const override { return kStrands; }
    int strandState(int strand) const override;
    float strandPhase(int strand) const override;
    int strandLayers(int strand) const override;
    bool strandPending(int strand) const override;
    int strandWave(int strand, float* out, int max) const override;
    float strandInput(int strand) const override;
    int strandPendingPress(int strand) const override;
    bool strandCanUndo(int strand) const override;
    bool strandCanRedo(int strand) const override;

    bool storeSessionAudio(const std::string& pathPrefix,
                           std::vector<std::pair<std::string, std::string>>& out) override;
    void loadSessionAudio() override;

    int audioTakeCount() const override { return kStrands; }
    bool audioTakeReady(int take) const override;
    std::string audioTakeName(int take) const override { return "Strand " + std::to_string(take + 1); }
    bool writeAudioTake(int take, const std::string& wavPath) override;

    static constexpr int kStrands = 4;
    enum TempoMode { kTempoPitch = 0, kTempoStretch = 1, kTempoOff = 2 };
    enum MonitorMode { kMonitorOff = 0, kMonitorMix = 1, kMonitorOut = 2 };

private:
    static constexpr double kMaxSeconds = 30.0;
    static constexpr int kPeakChunk = 512;
    static constexpr int kRebuildChunks = 16;
    static constexpr int kSlices = 4;
    static constexpr double kOnsetFloor = 0.25;
    static constexpr double kHoldSeconds = 0.35;
    static constexpr double kGraceBeats = 0.25;
    static constexpr int kSyncFree = 0, kSyncBeat = 1, kSyncBar = 2, kSyncFollow = 3;
    static constexpr int kSnapSpeed = 8;
    static constexpr std::int64_t kMaxFill = 64;
    static constexpr double kMinRatio = kMinTempoBpm / kMaxTempoBpm;
    static constexpr double kMaxRatio = kMaxTempoBpm / kMinTempoBpm;
    static constexpr double kUnityRatio = 1.0e-4;
    static constexpr double kNudgeBend = 0.03;

    enum class SState { Empty, Rec, Play, Dub, Stopped };
    enum class Pending { None, RecPress, StopPress, PlayPress, RecordPress, DubPress };

    struct Strand {
        std::array<std::vector<float>, 2> buf, undo;
        std::int64_t len = 0, recCount = 0;
        double pos = 0.0;
        double speed = 1.0;
        double recBpm = 0.0, tempoRatio = 1.0;
        bool stretch = false;
        SState state = SState::Empty;
        std::int64_t dubStart = 0, dubWritten = 0, snapCursor = 0, lastWrite = -1;
        double dubHead = 0.0;
        std::int64_t dubLag = 0, dubWarm = 0, dubTail = -1;
        bool dubFresh = false;
        int dubDir = 1;
        bool snapDone = true, undoReady = false, redo = false;
        bool reverse = false, oneShot = false, half = false;
        int layers = 0;
        Pending pending = Pending::None;
        double pendingBeat = 0.0;
        std::int64_t pendingLate = 0;
        int revPend = -1, halfPend = -1;
        double revBeat = 0.0, halfBeat = 0.0;
        bool prevRec = false, prevStop = false, prevUndo = false, prevClear = false;
        bool prevPlay = false, prevRedo = false;
        bool dubPress = false;
        std::int64_t heldSamples = 0;
        bool prevRecord = false, prevDub = false, dubHoldArmed = false;
        std::int64_t dubHeldSamples = 0;
        float level = 1.0f, levelPrev = 1.0f;
        std::vector<float> chunkPeak, undoChunkPeak;
        std::int64_t peakChunkAt = -1, rebuildAt = -1, undoLen = 0;
        int slice = 0, slicePrev = 0;
        int nudge = 0;
        bool waveDirty = true;
        float inPeak = 0.0f, inShow = 0.0f;
        std::array<std::atomic<float>, StrandWave::kWaveBins> uiWave {};
        std::atomic<int> uiWaveBins {0};
        std::atomic<float> uiInput {0.0f};
        std::atomic<int> uiState {0};
        std::atomic<float> uiPhase {-1.0f};
        std::atomic<int> uiLayers {0};
        std::atomic<bool> uiPending {false};
        std::atomic<int> uiPendingPress {0};
        std::atomic<bool> uiCanUndo {false}, uiCanRedo {false};
        std::string loadedUri;
    };

    struct StrandParams {
        ParamRef level, mute, solo, sync, rec, stop, play, undo, redo, clear, rev, half, shot, tempo, record, dub, monitor, slice, nudge;
    };
    static std::array<StrandParams, kStrands> makeStrandParams();

    void clearStrand(Strand& s);
    bool writeStrand(const Strand& s, const std::string& wavPath) const;
    void markPeak(Strand& s, std::int64_t at, float l, float r);
    static void holdSlice(Strand& s);
    static void publishWave(Strand& s);
    void rebuildPeaks(Strand& s);
    void recPress(Strand& s, std::int64_t late);
    void stopPress(Strand& s, std::int64_t late);
    void playPress(Strand& s, std::int64_t late);
    void recordPress(Strand& s, std::int64_t late);
    void overdubPress(Strand& s, std::int64_t late);
    void undoPress(Strand& s);
    void redoPress(Strand& s);
    void closeLoop(Strand& s, std::int64_t late, bool thenPlay);
    void beginDub(Strand& s);
    void finishDub(Strand& s);
    static void leaveDub(Strand& s);
    void snapshotAhead(Strand& s, std::int64_t upTo);
    void applyPending(Strand& s);
    void applyRev(Strand& s, bool v);
    void applyHalf(Strand& s, bool v);
    void anchorToTransport(Strand& s, int sync, double beats, double spb);
    void followTempo(Strand& s, int mode, bool canStretch) const;

    struct Grid {
        int sync = 0;
        double inUnit = 0.0, nextLine = 0.0;
        bool inGrace = false;
    };
    struct MasterPress {
        std::array<bool, kStrands> play {}, stop {};
        int playSync = 0, stopSync = 2;
    };
    static Grid gridAt(int sync, double beats, double beatsPerBar);
    Grid gridFor(int sync, int self, double beats, double beatsPerBar, double spb) const;
    int referenceFor(int self) const;
    int syncOf(int t);
    MasterPress masterPresses(bool playAll, bool stopAll);

    void runStrand(int t, float inL, float inR, int n, int numSamples, bool frozen,
                   float* const* out, int numOut, float& oL, float& oR);
    void dubSample(Strand& s, StrandStretch& st, float inL, float inR);
    void writeDub(Strand& s, std::int64_t index, float l, float r);
    static void advance(double& pos, double by, std::int64_t len, bool& wrapped);
    void publishUi();
    static int pressKind(const Strand& s);

    std::array<Strand, kStrands> strands_;
    std::array<StrandParams, kStrands> strandParams_ = makeStrandParams();
    std::array<StrandStretch, kStrands> stretch_;
    std::int64_t maxLoopSamples_ = 0;
    std::int64_t holdSamples_ = 0;
    std::int64_t dubLag_ = 0;
    double decay_ = 1.0;
    double tempoNow_ = 120.0;
    double barBeats_ = 4.0;
    bool prevRolling_ = false;
    bool prevPlayAll_ = false, prevStopAll_ = false;
    double expectBeats_ = 0.0;
};

}
