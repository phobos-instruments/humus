// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

#include "core/midi/MidiFormat.h"
#include "core/params/PatternRolls.h"
#include "gui/bricks/StepPlayhead.h"
#include "gui/editor/Words.h"
#include "gui/host/BrickHost.h"
#include "gui/host/PatternEdits.h"
#include "hum/PatternMatrix.h"
#include "hum/dsp/DspMath.h"
#include "io/PatchDocument.h"

namespace hum::grids {

inline constexpr Words kPlays{"pattern-step-grid.plays", "plays"};

class StepGridModel {
public:
    enum class Mode { Bassline, Arp };
    enum class Hit { None, Changed, BankMenu };
    enum class BankAction { Random, Clear, NudgeLeft, NudgeRight, CopyTo };
    enum class Stamp { Roll, Offbeat, Full, Clear, Random };

    static constexpr int kMinNote = kBasslineLowNote, kMaxNote = kBasslineHighNote;
    static constexpr int kRowH = 16;


    StepGridModel(BrickHost& host, PatternEdits& patterns, std::string organism, Mode mode, std::string nudgeParam, std::string transposeParam);

    Mode mode() const { return mode_; }

    int stepCount() const;
    std::vector<ArpStep> arpSteps() const { return patterns_.arpSteps(organism_); }
    std::vector<bool> arpUps() const { return patterns_.arpUps(organism_); }

    int nudge() const {
        return mode_ == Mode::Bassline ? (int) std::lround(host_.liveParamValue(organism_, nudgeParam_)) : 0;
    }

    int storedStep(int col) const;

    void setNudge(int to) {
        if (to != nudge()) host_.setParam(organism_, nudgeParam_, (double) to);
    }

    int columnAt(int x, int width) const;

    std::vector<BasslineStep> shownBassline() const { return rotatedSteps(patterns_.basslineSteps(organism_), nudge()); }

    static constexpr float kLabelGap = 4.0f;

    static float noteY(int note, int laneBottom);
    static float noteLabelTop(float barY, int laneBottom, float labelH);
    std::string noteReadout(int note) const;

    void pressed() { host_.pushUndo(); }

    Hit bassline(int x, int y, int width, int height, bool down, bool secondary);

    int dragColumn() const { return dragCol_; }
    bool releaseDrag();
    void beginSlide(int x);
    bool sliding() const { return sliding_; }
    void endSlide() { sliding_ = false; }
    void slideTo(int x, int width);

    int bank() const { return patterns_.bank(organism_); }

    void bankAction(BankAction action, int targetBank, Dice& dice);
    bool arp(int x, int y, int width, int height, bool down);
    void stamp(Stamp kind, Dice& dice);
    int playheadStep() const;
    bool followBank();

private:
    BrickHost& host_;
    PatternEdits& patterns_;
    std::string organism_;
    Mode mode_;
    std::string nudgeParam_, transposeParam_;
    int bank_ = -1;
    int shownNudge_ = 0;
    int dragCol_ = -1;
    int slideFrom_ = 0;
    int slideX_ = 0;
    bool sliding_ = false;
};

}
