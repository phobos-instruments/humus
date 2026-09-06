#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

#include "hum/dsp/DspMath.h"

namespace hum {
namespace ssd {

struct Config {
    int inputSize = 192;
    float minScale = 0.1484375f, maxScale = 0.75f;
    int strides[8] = {8, 16, 16, 16, 0, 0, 0, 0};
    int numLayers = 4;
    float offset = 0.5f;
    float scale = 192.0f;
    float scoreClip = 100.0f;
    float minScore = 0.5f;
    float minSuppressionIou = 0.3f;
    int rowWidth = 18;
    int kpCount = 7;
};

struct Anchor { float x, y, w, h; };

inline std::vector<Anchor> anchors(const Config& c = {}, bool coarseFirst = false) {
    std::vector<Anchor> out;
    std::vector<std::vector<Anchor>> byLayer;
    int layer = 0;
    while (layer < c.numLayers) {
        int same = layer;
        int perCell = 0;
        while (same < c.numLayers && c.strides[same] == c.strides[layer]) {
            perCell += 2;
            ++same;
        }
        const int stride = c.strides[layer];
        const int grid = (c.inputSize + stride - 1) / stride;
        std::vector<Anchor> block;
        for (int y = 0; y < grid; ++y)
            for (int x = 0; x < grid; ++x)
                for (int k = 0; k < perCell; ++k)
                    block.push_back({((float) x + c.offset) / (float) grid,
                                     ((float) y + c.offset) / (float) grid, 1.0f, 1.0f});
        byLayer.push_back(std::move(block));
        layer = same;
    }
    if (coarseFirst) std::reverse(byLayer.begin(), byLayer.end());
    for (auto& b : byLayer) out.insert(out.end(), b.begin(), b.end());
    return out;
}

inline int anchorCount(const Config& c = {}) { return (int) anchors(c).size(); }

struct Det {
    float score = 0.0f;
    float cx = 0.0f, cy = 0.0f, w = 0.0f, h = 0.0f;
    float kp[7][2]{};
};

inline float sigmoid(float v) { return 1.0f / (1.0f + std::exp(-v)); }

inline Det decodeOne(const float* row, float rawScore, const Anchor& a, const Config& c) {
    Det p;
    p.score = sigmoid(std::clamp(rawScore, -c.scoreClip, c.scoreClip));
    p.cx = row[0] / c.scale * a.w + a.x;
    p.cy = row[1] / c.scale * a.h + a.y;
    p.w = row[2] / c.scale * a.w;
    p.h = row[3] / c.scale * a.h;
    for (int k = 0; k < c.kpCount; ++k) {
        p.kp[k][0] = row[4 + 2 * k] / c.scale * a.w + a.x;
        p.kp[k][1] = row[5 + 2 * k] / c.scale * a.h + a.y;
    }
    return p;
}

inline float iou(const Det& a, const Det& b) {
    const float ax0 = a.cx - a.w * 0.5f, ax1 = a.cx + a.w * 0.5f;
    const float ay0 = a.cy - a.h * 0.5f, ay1 = a.cy + a.h * 0.5f;
    const float bx0 = b.cx - b.w * 0.5f, bx1 = b.cx + b.w * 0.5f;
    const float by0 = b.cy - b.h * 0.5f, by1 = b.cy + b.h * 0.5f;
    const float iw = std::min(ax1, bx1) - std::max(ax0, bx0);
    const float ih = std::min(ay1, by1) - std::max(ay0, by0);
    if (iw <= 0.0f || ih <= 0.0f) return 0.0f;
    const float inter = iw * ih;
    const float uni = a.w * a.h + b.w * b.h - inter;
    return uni > 0.0f ? inter / uni : 0.0f;
}

inline std::vector<Det> suppress(std::vector<Det> in, const Config& c, int maxOut) {
    std::sort(in.begin(), in.end(),
              [](const Det& a, const Det& b) { return a.score > b.score; });
    std::vector<Det> out;
    for (const auto& p : in) {
        if ((int) out.size() >= maxOut) break;
        bool covered = false;
        for (const auto& k : out) covered = covered || iou(p, k) > c.minSuppressionIou;
        if (!covered) out.push_back(p);
    }
    return out;
}

inline std::vector<Det> detections(const float* boxes, const float* scores,
                                   const std::vector<Anchor>& anch, const Config& c,
                                   int maxOut) {
    std::vector<Det> hits;
    for (size_t i = 0; i < anch.size(); ++i) {
        const Det p = decodeOne(boxes + i * (size_t) c.rowWidth, scores[i], anch[i], c);
        if (p.score >= c.minScore) hits.push_back(p);
    }
    return suppress(std::move(hits), c, maxOut);
}

struct Crop { float cx, cy, size, angle; };

inline Crop cropFor(const Det& p, float scale = 2.6f, float shift = 0.0f) {
    const float dx = p.kp[2][0] - p.kp[0][0], dy = p.kp[2][1] - p.kp[0][1];
    const float angle = kPiF * 0.5f - std::atan2(-dy, dx);
    const float size = std::max(p.w, p.h) * scale;
    return {p.cx - std::sin(angle) * size * shift,
            p.cy + std::cos(angle) * size * shift, size, angle};
}

}
}
