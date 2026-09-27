// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

#include "core/params/PatternRolls.h"
#include "gui/host/BrickHost.h"
#include "gui/host/PatternEdits.h"
#include "hum/Pattern.h"
#include "hum/PatternMatrix.h"
#include "hum/dsp/DspMath.h"
#include "io/PatchDocument.h"

namespace hum::grids {

class SequenceGridModel {
public:
    static constexpr int kRows = 8, kSteps = 16;
    static constexpr int kStepTicks = Pattern::kTicksPerBeat / 4;

    struct Params {
        std::string enablePrefix, notePrefix, velocityPrefix;
    };

    SequenceGridModel(BrickHost& host, PatternEdits& patterns, std::string organism, Params params,
                      int rows = kRows)
        : host_(host), patterns_(patterns), organism_(std::move(organism)), p_(std::move(params)),
          rows_(std::clamp(rows, 1, kRows)) {
        patterns_.ensureBanks(organism_, kRows, kPatternBanks);
    }

    int rows() const { return rows_; }

    static std::string param(const std::string& prefix, int row) { return prefix + std::to_string(row + 1); }

    bool enabled(int row) const { return host_.liveParamValue(organism_, param(p_.enablePrefix, row)) >= 0.5; }
    void toggleEnabled(int row) {
        host_.editParam(organism_, param(p_.enablePrefix, row), enabled(row) ? 0.0 : 1.0);
    }

    int note(int row) const { return (int) host_.liveParamValue(organism_, param(p_.notePrefix, row)); }
    std::string noteParam(int row) const { return param(p_.notePrefix, row); }
    bool stepNote(int row, int direction) {
        const int n = note(row);
        const int next = std::clamp(n + direction, 0, kMidiMax);
        if (next != n) host_.editParam(organism_, noteParam(row), (double) next);
        return true;
    }

    double velocity(int row) const { return host_.liveParamValue(organism_, param(p_.velocityPrefix, row)); }
    void setVelocity(int row, double v) { host_.editParam(organism_, param(p_.velocityPrefix, row), v); }

    std::vector<int> triggers(int row) const {
        const auto lanes = patterns_.triggerLanes(organism_);
        return row < (int) lanes.size() ? lanes[(size_t) row]->triggers : std::vector<int>{};
    }

    bool on(int row, int step) const {
        const auto t = triggers(row);
        return std::find(t.begin(), t.end(), step * kStepTicks) != t.end();
    }

    void beginPaint(int row, int step) {
        host_.pushUndo();
        painting_ = true;
        paintOn_ = !on(row, step);
        paintAt(row, step);
    }

    bool paintAt(int row, int step) {
        if (!painting_) return false;
        const int tick = step * kStepTicks;
        const bool has = on(row, step);
        if (paintOn_ && !has) patterns_.addTrigger(organism_, row, tick);
        else if (!paintOn_ && has) patterns_.removeTrigger(organism_, row, tick);
        else return false;
        return true;
    }

    void endPaint() { painting_ = false; }

    int bank() const { return patterns_.bank(organism_); }

    enum class BankAction { Random, Clear, CopyTo };

    void bankAction(BankAction action, int targetBank, Dice& dice) {
        const int cur = bank();
        host_.pushUndo();
        PatternSyncHold batch(host_);
        for (int row = 0; row < rows_; ++row) {
            std::vector<int> ticks;
            if (action == BankAction::Random) {
                const auto cells = randomTriggerRow(kSteps, 0.10 + dice.nextDouble() * 0.35, dice);
                for (int s = 0; s < kSteps && s < (int) cells.size(); ++s)
                    if (cells[(size_t) s]) ticks.push_back(s * kStepTicks);
            } else if (action == BankAction::CopyTo) {
                ticks = triggers(row);
            }
            patterns_.setLaneTriggers(organism_, action == BankAction::CopyTo ? targetBank : cur, row, ticks);
        }
    }

    std::string targetName(int row) const {
        std::string name;
        for (const auto& c : host_.model().midiConnections)
            if (c.src == organism_ && c.srcOutlet == row) {
                if (name.empty()) name = c.dst;
                else name += " +";
            }
        return name.empty() ? "(row " + std::to_string(row + 1) + ")" : name;
    }

    int playStep() const { return playStep_; }

    bool followPlayhead() {
        int step = -1;
        if (host_.isPlaying()) {
            const auto* cm = host_.model().byName(organism_);
            const int dur = cm && cm->pattern.present ? cm->pattern.duration : 0;
            if (dur > 0) {
                const double tick = std::fmod(host_.positionBeats() * Pattern::kTicksPerBeat, (double) dur);
                step = (int) (tick / kStepTicks) % kSteps;
            }
        }
        const int b = bank();
        if (step == playStep_ && b == bank_) return false;
        playStep_ = step;
        bank_ = b;
        return true;
    }

private:
    BrickHost& host_;
    PatternEdits& patterns_;
    std::string organism_;
    Params p_;
    int rows_ = kRows;
    bool painting_ = false;
    bool paintOn_ = true;
    int playStep_ = -1;
    int bank_ = -1;
};

}
