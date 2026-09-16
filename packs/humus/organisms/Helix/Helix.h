// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "hum/caps/Files.h"
#include "hum/Organism.h"
#include "hum/ParamRef.h"

namespace hum {

class Helix : public Organism, public StrandStatus, public SessionAudio {
public:
    int numAudioInputs() const override { return 2; }
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

    bool storeSessionAudio(const std::string& pathPrefix,
                           std::vector<std::pair<std::string, std::string>>& out) override;
    void loadSessionAudio() override;

    static constexpr int kStrands = 4;

private:
    static constexpr double kMaxSeconds = 30.0;
    static constexpr double kHoldSeconds = 0.35;
    static constexpr double kGraceBeats = 0.25;
    static constexpr int kSnapSpeed = 8;

    enum class SState { Empty, Rec, Play, Dub, Stopped };
    enum class Pending { None, RecPress, StopPress, PlayPress };

    struct Strand {
        std::array<std::vector<float>, 2> buf, undo;
        std::int64_t len = 0, recCount = 0;
        double pos = 0.0;
        double speed = 1.0;
        SState state = SState::Empty;
        std::int64_t dubStart = 0, dubWritten = 0, snapCursor = 0, lastWrite = -1;
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
        float level = 1.0f, levelPrev = 1.0f;
        std::atomic<int> uiState {0};
        std::atomic<float> uiPhase {-1.0f};
        std::atomic<int> uiLayers {0};
        std::atomic<bool> uiPending {false};
        std::string loadedUri;
    };

    struct StrandParams {
        ParamRef level, mute, solo, sync, rec, stop, play, undo, redo, clear, rev, half, shot;
    };
    static std::array<StrandParams, kStrands> makeStrandParams();

    void clearStrand(Strand& s);
    void recPress(Strand& s, std::int64_t late);
    void stopPress(Strand& s, std::int64_t late);
    void playPress(Strand& s, std::int64_t late);
    void undoPress(Strand& s);
    void redoPress(Strand& s);
    void closeLoop(Strand& s, std::int64_t late, bool thenPlay);
    void beginDub(Strand& s);
    void snapshotAhead(Strand& s, std::int64_t upTo);
    void applyPending(Strand& s);
    void applyRev(Strand& s, bool v);
    void applyHalf(Strand& s, bool v);
    void anchorToTransport(Strand& s, int sync, double beats, double spb);

    std::array<Strand, kStrands> strands_;
    std::array<StrandParams, kStrands> strandParams_ = makeStrandParams();
    std::int64_t maxLoopSamples_ = 0;
    std::int64_t holdSamples_ = 0;
    double decay_ = 1.0;
    bool prevRolling_ = false;
    double expectBeats_ = 0.0;
};

}
