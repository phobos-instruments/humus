#pragma once
#include <algorithm>
#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

struct CompressorCore {
    void set(double thresholdDb, double ratio, double kneeDb, double attackMs,
             double releaseMs, double holdMs, double sampleRate) {
        thrDb_ = thresholdDb;
        ratio_ = std::max(1.0, ratio);
        knee_ = std::max(0.0, kneeDb);
        atk_ = smoothCoeff(attackMs, sampleRate);
        rel_ = smoothCoeff(releaseMs, sampleRate);
        holdN_ = (int) (holdMs * 0.001 * sampleRate);
    }

    void reset() {
        envDb_ = 0.0;
        hold_ = 0;
    }

    double process(double level) {
        const double levelDb = linToDb(level);
        const double over = levelDb - thrDb_;
        double outDb;
        if (knee_ > 0.0 && 2.0 * over < knee_ && 2.0 * over > -knee_)
            outDb = levelDb
                  + (1.0 / ratio_ - 1.0) * std::pow(over + knee_ / 2.0, 2.0) / (2.0 * knee_);
        else if (over > 0.0)
            outDb = thrDb_ + over / ratio_;
        else
            outDb = levelDb;
        const double target = outDb - levelDb;
        if (target < envDb_) {
            envDb_ = atk_ * envDb_ + (1.0 - atk_) * target;
            hold_ = holdN_;
        } else if (hold_ > 0) {
            --hold_;
        } else {
            envDb_ = rel_ * envDb_ + (1.0 - rel_) * target;
        }
        return dbToLin(envDb_);
    }

    double autoMakeup() const { return dbToLin(0.5 * (1.0 - 1.0 / ratio_) * (-thrDb_)); }

    double reductionDb() const { return envDb_; }

private:
    double thrDb_ = 0.0, ratio_ = 2.0, knee_ = 6.0;
    double atk_ = 0.0, rel_ = 0.0;
    int holdN_ = 0, hold_ = 0;
    double envDb_ = 0.0;
};

struct LimiterCore {
    void set(double thresholdLin, double releaseMs, double holdMs, double sampleRate) {
        thr_ = std::max(1e-6, thresholdLin);
        rel_ = smoothCoeff(releaseMs, sampleRate);
        holdN_ = (int) (holdMs * 0.001 * sampleRate);
    }

    void reset() {
        gain_ = 1.0;
        hold_ = 0;
    }

    double process(double peak) {
        const double target = peak > thr_ ? thr_ / peak : 1.0;
        if (target < gain_) { gain_ = target; hold_ = holdN_; }
        else if (hold_ > 0) { --hold_; }
        else                { gain_ = rel_ * gain_ + (1.0 - rel_) * target; }
        return gain_;
    }

    double threshold() const { return thr_; }
    double reductionDb() const { return linToDb(gain_); }

private:
    double thr_ = 1.0, rel_ = 0.0;
    int holdN_ = 0, hold_ = 0;
    double gain_ = 1.0;
};

struct GateCore {
    void set(double openThresh, double closeThresh, double floorGain, bool duck,
             double attackMs, double releaseMs, double holdMs, double sampleRate) {
        openT_ = openThresh;
        closeT_ = std::min(closeThresh, openThresh);
        floor_ = floorGain;
        duck_ = duck;
        atk_ = smoothCoeff(attackMs, sampleRate);
        rel_ = smoothCoeff(releaseMs, sampleRate);
        holdN_ = (int) (holdMs * 0.001 * sampleRate);
    }

    void reset() {
        gain_ = 1.0;
        open_ = false;
        hold_ = 0;
    }

    double process(double peak) {
        if (peak > openT_) { open_ = true; hold_ = holdN_; }
        else if (peak < closeT_) { if (hold_ > 0) --hold_; else open_ = false; }
        const double target = duck_ ? (open_ ? floor_ : 1.0) : (open_ ? 1.0 : floor_);
        const double c = target > gain_ ? atk_ : rel_;
        gain_ = c * gain_ + (1.0 - c) * target;
        return gain_;
    }
    double reductionDb() const { return linToDb(gain_); }

    bool isOpen() const { return open_; }

private:
    double openT_ = 0.2, closeT_ = 0.1, floor_ = 0.0;
    bool duck_ = false, open_ = false;
    double atk_ = 0.0, rel_ = 0.0;
    int holdN_ = 0, hold_ = 0;
    double gain_ = 1.0;
};

}
