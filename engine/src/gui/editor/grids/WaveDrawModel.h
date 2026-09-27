// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "gui/host/ModelHost.h"
#include "hum/dsp/WaveTable.h"
#include "hum/dsp/WaveWarp.h"

namespace hum::grids {

class WaveDrawModel {
public:
    static constexpr int kMaxFrames = 16;

    struct Params {
        std::string table, position, warp, warpMode;
    };

    WaveDrawModel(ModelHost& host, std::string organism, Params params);

    std::string liveText() const { return host_.liveParamText(organism_, p_.table); }
    int frameCount() const { return frameCount_; }
    int activePreset() const { return activePreset_; }
    bool drawing() const { return drawing_; }
    const float* frame(int f) const { return frames_[f]; }
    int warpMode() const { return (int) host_.liveParamValue(organism_, p_.warpMode); }
    float warpAmount() const { return (float) host_.liveParamValue(organism_, p_.warp); }

    int currentFrame() const;
    float valueAt(double phase) const;
    void blended(float* out) const;
    std::string sourceText() const;
    void choosePreset(int i);
    void randomize(std::uint32_t seed);
    bool pickFrame(int x, int left, int width);
    void beginDrawing();
    void drawAt(int x, int y, int left, int width, int centreY, int height);
    bool endDrawing();
    bool addFrame();
    bool removeFrame();
    void seedCycle(const float* samples, int count);
    void seedBytes(const std::uint8_t* bytes, int count);
    bool seedSignal(const std::vector<float>& samples, std::string kind, std::string label);
    void seedAudio(const std::vector<float>& mono, double sampleRate);
    void seedTable(const std::vector<float>& mono, int frameLen);
    void pull(const std::string& text);
    bool poll();

private:
    void blend(int& f0, int& f1, float& fr) const;
    void clearSeed();
    void seeded();
    void push();

    ModelHost& host_;
    std::string organism_;
    Params p_;
    std::string seededKind_, seededLabel_;
    float frames_[kMaxFrames][kWaveTableLen] = {};
    int frameCount_ = 1;
    double pos_ = 0.0;
    std::string cachedText_;
    int activePreset_ = -1;
    int lastWm_ = -1;
    float lastWa_ = -1.0f;
    bool drawing_ = false;
    int lastIdx_ = -1;
    float lastVal_ = 0.0f;
};

}
