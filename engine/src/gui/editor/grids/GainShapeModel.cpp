// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/grids/GainShapeModel.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

namespace hum::grids {

GainShapeModel::GainShapeModel(BrickHost& host, std::string organism, std::string param)
    : host_(host), organism_(std::move(organism)), param_(std::move(param)) {
    if (!decodeGainShape(host_.liveParamText(organism_, param_).c_str(), shape_)) shape_ = gainShapePreset(0);
    matchPreset();
}

void GainShapeModel::choosePreset(int i) {
    shape_ = gainShapePreset(i);
    activePreset_ = i;
    push();
}

void GainShapeModel::beginDrawing() {
    drawing_ = true;
    for (int i = 0; i < kSlots; ++i) slots_[i] = (float) shape_.eval((double) i / (kSlots - 1));
    lastSlot_ = -1;
}

int GainShapeModel::slotAt(int x, int left, int width) {
    const int s = (int) std::lround((double) (x - left) / width * (kSlots - 1));
    return s < 0 ? 0 : (kSlots - 1 < s ? kSlots - 1 : s);
}

void GainShapeModel::drawAt(int slot, float v) {
    if (lastSlot_ < 0) {
        slots_[slot] = v;
    } else {
        const int a = std::min(lastSlot_, slot), b = std::max(lastSlot_, slot);
        for (int i = a; i <= b; ++i) {
            const float t = b == a ? 1.0f : (float) (i - a) / (float) (b - a);
            const float from = slot >= lastSlot_ ? lastVal_ : v;
            const float to = slot >= lastSlot_ ? v : lastVal_;
            slots_[i] = from + (to - from) * t;
        }
    }
    lastSlot_ = slot;
    lastVal_ = v;
}

bool GainShapeModel::endDrawing() {
    if (!drawing_) return false;
    drawing_ = false;
    simplify();
    activePreset_ = -1;
    push();
    return true;
}

bool GainShapeModel::followPlayhead(int width) {
    float phase = -1.0f, gain = playGain_;
    if (host_.isPlaying()) {
        const live::Controls controls(host_, organism_);
        phase = controls.valueOr("phase", phase);
        gain = controls.valueOr("gain", gain);
    }
    const int w = std::max(1, width);
    if ((int) (phase * w) == (int) (playhead_ * w) && std::abs(gain - playGain_) < 0.004f) return false;
    playhead_ = phase;
    playGain_ = gain;
    return true;
}

double GainShapeModel::slotValue(double phase) const {
    const double f = phase * (kSlots - 1);
    const int raw = (int) f;
    const int i = raw < 0 ? 0 : (kSlots - 2 < raw ? kSlots - 2 : raw);
    const double t = f - i;
    return slots_[i] * (1.0 - t) + slots_[i + 1] * t;
}

void GainShapeModel::matchPreset() {
    activePreset_ = -1;
    for (int i = 0; i < kGainShapePresets; ++i)
        if (encodeGainShape(shape_) == encodeGainShape(gainShapePreset(i))) activePreset_ = i;
}

void GainShapeModel::replace(const GainShape& next) {
    shape_ = next;
    matchPreset();
    push();
}

void GainShapeModel::simplify() {
    double eps = 0.008;
    for (;;) {
        GainShape g;
        int anchor = 0;
        g.add(0.0f, slots_[0]);
        while (anchor < kSlots - 1) {
            int j = anchor + 1;
            for (int cand = anchor + 2; cand < kSlots; ++cand) {
                bool ok = true;
                for (int k = anchor + 1; k < cand && ok; ++k) {
                    const double t = (double) (k - anchor) / (cand - anchor);
                    const double lin = slots_[anchor] + (slots_[cand] - slots_[anchor]) * t;
                    ok = std::abs(lin - slots_[k]) <= eps;
                }
                if (!ok) break;
                j = cand;
            }
            if (g.n >= GainShape::kMaxPoints) break;
            g.add((float) j / (kSlots - 1), slots_[j]);
            anchor = j;
        }
        if (anchor >= kSlots - 1 && g.n <= GainShape::kMaxPoints) {
            shape_ = g;
            return;
        }
        eps *= 1.7;
    }
}

}
