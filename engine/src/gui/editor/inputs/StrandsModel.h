// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <utility>

#include "gui/editor/LiveControls.h"
#include "gui/host/ModelHost.h"
#include "hum/caps/Files.h"

namespace hum::input {

class StrandsModel {
public:
    struct Strand {
        int state = 0;
        float phase = -1.0f;
        int layers = 0;
        bool pending = false;
        bool audible = true;
    };

    StrandsModel(ModelHost& host, std::string organism, std::string filePrefix, std::string mutePrefix,
                 std::string soloPrefix, std::string slicePrefix, std::string nudgePrefix = {})
        : host_(host), organism_(std::move(organism)), filePrefix_(std::move(filePrefix)),
          mutePrefix_(std::move(mutePrefix)), soloPrefix_(std::move(soloPrefix)),
          slicePrefix_(std::move(slicePrefix)), nudgePrefix_(std::move(nudgePrefix)) {
        lastState_.fill(-1);
        lastLayers_.fill(-1);
        lastPhase_.fill(-1);
        lastPending_.fill(false);
    }

    int count() const {
        auto* st = status();
        return st ? st->strandCount() : 4;
    }

    Strand strand(int i) const {
        auto* st = status();
        Strand s;
        s.state = st ? st->strandState(i) : 0;
        s.phase = st ? st->strandPhase(i) : -1.0f;
        s.layers = st ? st->strandLayers(i) : 0;
        s.pending = st && st->strandPending(i);
        const std::string k = std::to_string(i + 1);
        s.audible = !on(mutePrefix_ + k) && (!anySolo() || on(soloPrefix_ + k));
        return s;
    }

    bool blinkLit() const { return (blink_ & 4) != 0; }

    int waveOf(int i, float* out, int max) const {
        auto* w = live::source<StrandWave>(host_, organism_);
        return w ? w->strandWave(i, out, max) : 0;
    }

    float inputOf(int i) const {
        auto* w = live::source<StrandWave>(host_, organism_);
        return w ? w->strandInput(i) : 0.0f;
    }

    bool poll() {
        auto* st = status();
        bool moved = false;
        bool anyPending = false;
        for (int i = 0; i < (int) lastState_.size(); ++i) {
            const int state = st ? st->strandState(i) : 0;
            const int layers = st ? st->strandLayers(i) : 0;
            const std::string k = std::to_string(i + 1);
            const int ms = (on(mutePrefix_ + k) ? 1 : 0) + (on(soloPrefix_ + k) ? 2 : 0);
            const bool pending = st && st->strandPending(i);
            const float ph = st ? st->strandPhase(i) : -1.0f;
            const int phq = ph < 0.0f ? -1 : (int) (ph * 96.0f);
            const auto j = (size_t) i;
            moved = moved || state != lastState_[j] || layers != lastLayers_[j] || pending != lastPending_[j]
                    || phq != lastPhase_[j] || ms != lastMuteSolo_[j];
            lastState_[j] = state;
            lastLayers_[j] = layers;
            lastMuteSolo_[j] = ms;
            lastPending_[j] = pending;
            lastPhase_[j] = phq;
            anyPending = anyPending || pending;
            const int lev = (int) (inputOf(i) * 24.0f);
            moved = moved || lev != lastInput_[j];
            lastInput_[j] = lev;
        }
        if (anyPending) ++blink_;
        return moved || anyPending;
    }

    int strandAt(int x, int width) const {
        const int n = count();
        const int v = (int) ((float) x / ((float) width / (float) n));
        return v < 0 ? 0 : (n - 1 < v ? n - 1 : v);
    }

    static constexpr int kSlices = 4;

    bool slices() const { return !slicePrefix_.empty(); }

    int sliceOf(int strand) const {
        if (!slices()) return 0;
        return (int) std::lround(host_.liveParamValue(organism_, slicePrefix_ + std::to_string(strand + 1)));
    }

    void setSlice(int strand, int quarter) {
        if (!slices()) return;
        host_.setParam(organism_, slicePrefix_ + std::to_string(strand + 1), (double) quarter);
    }

    bool nudges() const { return !nudgePrefix_.empty(); }

    int nudgeOf(int strand) const {
        if (!nudges()) return 0;
        return (int) std::lround(host_.liveParamValue(organism_, nudgePrefix_ + std::to_string(strand + 1)));
    }

    void setNudge(int strand, int direction) {
        if (!nudges()) return;
        host_.setParam(organism_, nudgePrefix_ + std::to_string(strand + 1), (double) direction);
    }

    void dropFile(int strand, const std::string& path) {
        host_.setParamText(organism_, filePrefix_ + std::to_string(strand + 1), path);
        if (auto* sa = live::source<SessionAudio>(host_, organism_)) sa->loadSessionAudio();
    }

private:
    StrandStatus* status() const { return live::source<StrandStatus>(host_, organism_); }
    bool on(const std::string& param) const { return host_.liveParamValue(organism_, param) >= 0.5; }

    bool anySolo() const {
        for (int i = 0; i < count(); ++i)
            if (on(soloPrefix_ + std::to_string(i + 1))) return true;
        return false;
    }

    ModelHost& host_;
    std::string organism_, filePrefix_, mutePrefix_, soloPrefix_, slicePrefix_, nudgePrefix_;
    std::array<int, 4> lastState_{}, lastLayers_{}, lastPhase_{}, lastMuteSolo_{}, lastInput_{};
    std::array<bool, 4> lastPending_{};
    int blink_ = 0;
};

}
