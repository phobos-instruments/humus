// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/grids/WaveDrawModel.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "hum/dsp/WaveTableFile.h"

namespace hum::grids {

WaveDrawModel::WaveDrawModel(ModelHost& host, std::string organism, Params params)
    : host_(host), organism_(std::move(organism)), p_(std::move(params)) {
    pull(liveText());
}

int WaveDrawModel::currentFrame() const {
    const int f = (int) std::lround(pos_ * (frameCount_ - 1));
    return std::clamp(f, 0, frameCount_ - 1);
}

float WaveDrawModel::valueAt(double phase) const {
    int f0, f1;
    float fr;
    blend(f0, f1, fr);
    const double f = phase * (kWaveTableLen - 1);
    const int i = std::clamp((int) f, 0, kWaveTableLen - 2);
    const float t = (float) (f - i);
    auto samp = [&](int fi) { return frames_[fi][i] * (1.0f - t) + frames_[fi][i + 1] * t; };
    return samp(f0) + fr * (samp(f1) - samp(f0));
}

void WaveDrawModel::blended(float* out) const {
    int f0, f1;
    float fr;
    blend(f0, f1, fr);
    for (int k = 0; k < kWaveTableLen; ++k) out[k] = frames_[f0][k] + fr * (frames_[f1][k] - frames_[f0][k]);
}

std::string WaveDrawModel::sourceText() const {
    std::string frames = frameCount_ > 1
                             ? "frame " + std::to_string(currentFrame() + 1) + " / " + std::to_string(frameCount_)
                             : std::string("1 frame  -  Seed a file for a wavetable");
    if (seededKind_.empty()) return frames;
    std::string from = seededKind_;
    if (!seededLabel_.empty()) from += ": " + seededLabel_;
    return from + "  -  " + frames;
}

void WaveDrawModel::choosePreset(int i) {
    waveTablePreset(i, frames_[currentFrame()], kWaveTableLen);
    activePreset_ = i;
    push();
}

void WaveDrawModel::randomize(std::uint32_t seed) {
    waveTableRandom(seed, frames_[currentFrame()], kWaveTableLen);
    activePreset_ = -1;
    push();
}

bool WaveDrawModel::pickFrame(int x, int left, int width) {
    if (frameCount_ <= 1) return false;
    const int n = frameCount_;
    const int f = std::clamp((x - left) * n / std::max(1, width), 0, n - 1);
    host_.setParam(organism_, p_.position, n > 1 ? (double) f / (n - 1) : 0.0);
    return true;
}

void WaveDrawModel::beginDrawing() {
    drawing_ = true;
    lastIdx_ = -1;
}

void WaveDrawModel::drawAt(int x, int y, int left, int width, int centreY, int height) {
    const int idx = std::clamp((int) std::lround((double) (x - left) / width * (kWaveTableLen - 1)), 0,
                               kWaveTableLen - 1);
    const float v = (float) std::clamp(((double) centreY - y) / (height * 0.46), -1.0, 1.0);
    float* f = frames_[currentFrame()];
    if (lastIdx_ < 0) {
        f[idx] = v;
    } else {
        const int a = std::min(lastIdx_, idx), b = std::max(lastIdx_, idx);
        for (int i = a; i <= b; ++i) {
            const float t = b == a ? 1.0f : (float) (i - a) / (float) (b - a);
            const float from = idx >= lastIdx_ ? lastVal_ : v;
            const float to = idx >= lastIdx_ ? v : lastVal_;
            f[i] = from + (to - from) * t;
        }
    }
    lastIdx_ = idx;
    lastVal_ = v;
}

bool WaveDrawModel::endDrawing() {
    if (!drawing_) return false;
    drawing_ = false;
    activePreset_ = -1;
    push();
    return true;
}

bool WaveDrawModel::addFrame() {
    if (frameCount_ >= kMaxFrames) return false;
    const int at = currentFrame();
    for (int f = frameCount_; f > at + 1; --f) std::copy(frames_[f - 1], frames_[f - 1] + kWaveTableLen, frames_[f]);
    std::copy(frames_[at], frames_[at] + kWaveTableLen, frames_[at + 1]);
    ++frameCount_;
    push();
    host_.setParam(organism_, p_.position, frameCount_ > 1 ? (double) (at + 1) / (frameCount_ - 1) : 0.0);
    return true;
}

bool WaveDrawModel::removeFrame() {
    if (frameCount_ <= 1) return false;
    const int at = currentFrame();
    for (int f = at; f + 1 < frameCount_; ++f) std::copy(frames_[f + 1], frames_[f + 1] + kWaveTableLen, frames_[f]);
    --frameCount_;
    push();
    const int nc = std::min(at, frameCount_ - 1);
    host_.setParam(organism_, p_.position, frameCount_ > 1 ? (double) nc / (frameCount_ - 1) : 0.0);
    return true;
}

void WaveDrawModel::seedCycle(const float* samples, int count) {
    clearSeed();
    waveTableFromCycle(samples, count, frames_[0], kWaveTableLen);
    frameCount_ = 1;
    seeded();
}

void WaveDrawModel::seedBytes(const std::uint8_t* bytes, int count) {
    clearSeed();
    waveTableFromBytes(bytes, count, frames_[0], kWaveTableLen);
    frameCount_ = 1;
    seeded();
}

bool WaveDrawModel::seedSignal(const std::vector<float>& samples, std::string kind, std::string label) {
    const int n = (int) samples.size();
    if (n < kWaveTableLen / 4) return false;
    clearSeed();
    const int count = std::clamp(n / (kWaveTableLen / 4), 1, kMaxFrames);
    const int win = n / count;
    for (int f = 0; f < count; ++f) waveTableFromCycle(samples.data() + (size_t) f * win, win, frames_[f], kWaveTableLen);
    frameCount_ = count;
    waveTableAlignFrames(&frames_[0][0], frameCount_, kWaveTableLen);
    seededKind_ = std::move(kind);
    seededLabel_ = std::move(label);
    seeded();
    return true;
}

void WaveDrawModel::seedAudio(const std::vector<float>& mono, double sampleRate) {
    clearSeed();
    const int take = (int) mono.size();
    frameCount_ = waveTableFramesFromSignal(mono.data(), take, sampleRate, &frames_[0][0], kWaveTableLen, kMaxFrames);
    waveTableAlignFrames(&frames_[0][0], frameCount_, kWaveTableLen);
    if (frameCount_ <= 0) {
        waveTableFromCycle(mono.data(), std::min(take, 4096), frames_[0], kWaveTableLen);
        frameCount_ = 1;
    }
    seeded();
}

void WaveDrawModel::seedTable(const std::vector<float>& mono, int frameLen) {
    clearSeed();
    frameCount_ = waveTableFramesFromTable(mono.data(), (int) mono.size(), frameLen, &frames_[0][0], kWaveTableLen, kMaxFrames);
    if (frameCount_ <= 0) {
        seedAudio(mono, kDefaultSampleRate);
        return;
    }
    seededKind_ = "wavetable";
    seededLabel_ = std::to_string((int) mono.size() / frameLen) + " frames in the file";
    seeded();
}

void WaveDrawModel::pull(const std::string& text) {
    cachedText_ = text;
    clearSeed();
    const int n = decodeWaveFrames(text.c_str(), &frames_[0][0], kWaveTableLen, kMaxFrames);
    frameCount_ = n > 0 ? n : 1;
    waveTableAlignFrames(&frames_[0][0], frameCount_, kWaveTableLen);
    if (n <= 0) waveTablePreset(0, frames_[0], kWaveTableLen);
    activePreset_ = -1;
    if (frameCount_ == 1)
        for (int i = 0; i < kWaveTablePresets; ++i) {
            float t[kWaveTableLen];
            waveTablePreset(i, t, kWaveTableLen);
            const bool empty = text.empty() && i == 0;
            if (empty || text == encodeWaveTable(t, kWaveTableLen)) {
                activePreset_ = i;
                break;
            }
        }
}

bool WaveDrawModel::poll() {
    if (drawing_) return false;
    bool changed = false;
    if (const auto text = liveText(); text != cachedText_) {
        pull(text);
        changed = true;
    }
    const int wm = warpMode();
    const float wa = warpAmount();
    const double pos = host_.liveParamValue(organism_, p_.position);
    if (wm != lastWm_ || std::abs(wa - lastWa_) > 1e-3f || std::abs(pos - pos_) > 1e-3) {
        lastWm_ = wm;
        lastWa_ = wa;
        pos_ = pos;
        changed = true;
    }
    return changed;
}

void WaveDrawModel::blend(int& f0, int& f1, float& fr) const {
    const double fp = pos_ * (frameCount_ - 1);
    f0 = std::clamp((int) fp, 0, frameCount_ - 1);
    f1 = std::min(f0 + 1, frameCount_ - 1);
    fr = (float) (fp - f0);
}

void WaveDrawModel::clearSeed() {
    seededKind_.clear();
    seededLabel_.clear();
}

void WaveDrawModel::seeded() {
    activePreset_ = -1;
    push();
}

void WaveDrawModel::push() {
    cachedText_ = encodeWaveFrames(&frames_[0][0], frameCount_, kWaveTableLen);
    host_.setParamText(organism_, p_.table, cachedText_);
}

}
