// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/grids/StepGridModel.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace hum::grids {

StepGridModel::StepGridModel(BrickHost& host, PatternEdits& patterns, std::string organism, Mode mode, std::string nudgeParam, std::string transposeParam)
    : host_(host), patterns_(patterns), organism_(std::move(organism)), mode_(mode), nudgeParam_(std::move(nudgeParam)), transposeParam_(std::move(transposeParam)) {}

int StepGridModel::stepCount() const {
    return mode_ == Mode::Bassline ? (int) patterns_.basslineSteps(organism_).size()
                                   : (int) patterns_.arpSteps(organism_).size();
}

int StepGridModel::storedStep(int col) const {
    const int n = std::max(1, stepCount());
    return ((col - nudge()) % n + n) % n;
}

int StepGridModel::columnAt(int x, int width) const {
    const int n = std::max(1, stepCount());
    return std::clamp(x * n / std::max(1, width), 0, n - 1);
}

float StepGridModel::noteY(int note, int laneBottom) {
    const float range = (float) (kMaxNote - kMinNote);
    return laneBottom * (1.0f - (std::clamp(note, kMinNote, kMaxNote) - kMinNote) / range);
}

float StepGridModel::noteLabelTop(float barY, int laneBottom, float labelH) {
    const float below = barY + kLabelGap;
    if (below + labelH <= (float) laneBottom) return below;
    return std::max(0.0f, barY - kLabelGap - labelH);
}

std::string StepGridModel::noteReadout(int note) const {
    const int transpose = (int) std::lround(host_.liveParamValue(organism_, transposeParam_));
    const auto shown = midiNoteName(note);
    if (transpose == 0) return shown;
    return shown + "  " + say(host_, kPlays) + " " + midiNoteName(std::clamp(note + transpose, 0, kMidiMax));
}

StepGridModel::Hit StepGridModel::bassline(int x, int y, int width, int height, bool down, bool secondary) {
    const auto steps = patterns_.basslineSteps(organism_);
    if (steps.empty()) return Hit::None;
    const int col = columnAt(x, width);
    const int i = storedStep(col);
    auto s = steps[(size_t) i];
    const int laneBottom = height - 2 * kRowH;
    if (secondary && !down) return Hit::None;
    if (y < laneBottom) {
        if (secondary) {
            s.gate = !s.gate;
        } else {
            const float t = std::clamp(1.0f - y / (float) laneBottom, 0.0f, 1.0f);
            s.note = kMinNote + (int) std::lround(t * (kMaxNote - kMinNote));
            s.gate = true;
            dragCol_ = col;
        }
    } else if (down) {
        if (secondary) return Hit::BankMenu;
        const int row = (y - laneBottom) / kRowH;
        if (row == 0) s.accent = !s.accent;
        else s.slide = !s.slide;
    } else {
        return Hit::None;
    }
    patterns_.setBasslineStep(organism_, i, s);
    return Hit::Changed;
}

bool StepGridModel::releaseDrag() {
    if (dragCol_ < 0) return false;
    dragCol_ = -1;
    return true;
}

void StepGridModel::beginSlide(int x) {
    slideFrom_ = nudge();
    slideX_ = x;
    sliding_ = stepCount() > 0;
}

void StepGridModel::slideTo(int x, int width) {
    const float cw = width / (float) std::max(1, stepCount());
    setNudge(wrappedNudge(slideFrom_, (int) std::lround((float) (x - slideX_) / cw), stepCount()));
}

void StepGridModel::bankAction(BankAction action, int targetBank, Dice& dice) {
    const int cur = bank();
    host_.pushUndo();
    if (action == BankAction::NudgeLeft || action == BankAction::NudgeRight) {
        setNudge(wrappedNudge(nudge(), action == BankAction::NudgeRight ? 1 : -1, stepCount()));
        return;
    }
    auto steps = patterns_.basslineSteps(organism_, cur);
    if (action == BankAction::Random) steps = randomBassline((int) steps.size(), basslineRoot(steps), dice);
    else if (action == BankAction::Clear) steps.assign(steps.size(), BasslineStep{});
    patterns_.setBasslineSteps(organism_, action == BankAction::CopyTo ? targetBank : cur, steps);
}

bool StepGridModel::arp(int x, int y, int width, int height, bool down) {
    if (!down) return false;
    auto steps = patterns_.arpSteps(organism_);
    if (steps.empty()) return false;
    const int i = columnAt(x, width);
    if (y >= height - 2 * kRowH && y < height - kRowH) {
        const auto ups = patterns_.arpUps(organism_);
        patterns_.setArpUp(organism_, i, !((size_t) i < ups.size() && ups[(size_t) i]));
        return true;
    }
    auto s = steps[(size_t) i];
    if (y >= height - kRowH) s.tie = !s.tie;
    else s.trigger = !s.trigger;
    patterns_.setArpStep(organism_, i, s);
    return true;
}

void StepGridModel::stamp(Stamp kind, Dice& dice) {
    auto steps = patterns_.arpSteps(organism_);
    const auto ups = patterns_.arpUps(organism_);
    const int n = (int) steps.size();
    const auto rolled = randomTriggerRow(n, 0.35 + dice.nextDouble() * 0.35, dice);
    host_.pushUndo();
    for (int i = 0; i < n; ++i) {
        auto s = steps[(size_t) i];
        s.trigger = kind == Stamp::Roll      ? (i % 4) != 0
                  : kind == Stamp::Offbeat   ? (i % 4) == 2
                  : kind == Stamp::Full      ? true
                                             : kind == Stamp::Random && rolled[(size_t) i];
        s.tie = kind == Stamp::Random && dice.nextDouble() < 0.12;
        patterns_.setArpStep(organism_, i, s);
        const bool haveUp = (size_t) i < ups.size() && ups[(size_t) i];
        const bool wantUp = kind == Stamp::Random && s.trigger && dice.nextDouble() < 0.18;
        if (wantUp != haveUp) patterns_.setArpUp(organism_, i, wantUp);
    }
}

int StepGridModel::playheadStep() const {
    const auto* cm = host_.model().byName(organism_);
    return cm == nullptr ? -1
                         : stepPlayhead(host_.isPlaying(), host_.positionBeats(), cm->pattern.matrixResolution,
                                        stepCount());
}

bool StepGridModel::followBank() {
    if (mode_ != Mode::Bassline) return false;
    const int cur = bank(), shift = nudge();
    if (cur == bank_ && shift == shownNudge_) return false;
    bank_ = cur;
    shownNudge_ = shift;
    return true;
}

}
