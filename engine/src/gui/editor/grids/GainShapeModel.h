// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

#include "gui/editor/LiveControls.h"
#include "gui/host/BrickHost.h"
#include "hum/dsp/GainShape.h"

namespace hum::grids {

class GainShapeModel {
public:
    static constexpr int kSlots = 129;
    static constexpr double kNudge = 1.0 / 32.0;
    static constexpr double kBigNudge = 1.0 / 8.0;

    GainShapeModel(BrickHost& host, std::string organism, std::string param);

    const GainShape& shape() const { return shape_; }
    int activePreset() const { return activePreset_; }
    bool drawing() const { return drawing_; }
    float playhead() const { return playhead_; }
    float playGain() const { return playGain_; }

    double valueAt(double phase) const { return drawing_ ? slotValue(phase) : shape_.eval(phase); }

    void reverse() { replace(reversedGainShape(shape_)); }
    void invert() { replace(invertedGainShape(shape_)); }
    void nudge(double delta) { replace(rotatedGainShape(shape_, delta)); }

    void choosePreset(int i);
    void beginDrawing();
    static int slotAt(int x, int left, int width);

    static float levelAt(int y, int top, int height) {
        return (float) std::clamp(1.0 - (double) (y - top) / height, 0.0, 1.0);
    }

    void drawAt(int slot, float v);
    bool endDrawing();
    bool followPlayhead(int width);

private:
    double slotValue(double phase) const;
    void matchPreset();
    void replace(const GainShape& next);
    void simplify();

    void push() { host_.setParamText(organism_, param_, encodeGainShape(shape_)); }

    BrickHost& host_;
    std::string organism_, param_;
    GainShape shape_;
    int activePreset_ = -1;
    bool drawing_ = false;
    float slots_[kSlots] = {};
    int lastSlot_ = -1;
    float lastVal_ = 0.0f;
    float playhead_ = -1.0f;
    float playGain_ = 1.0f;
};

}
