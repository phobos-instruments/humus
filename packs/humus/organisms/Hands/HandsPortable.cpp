#include "Hands/HandsPortable.h"

#include <algorithm>
#include <cmath>
#include <mutex>

#include "core/AppPaths.h"

#include "Hands/HandNet.h"
#include "Hands/PalmDecode.h"

namespace hum {
namespace {

struct Nets {
    HandNet detector, landmarks;
    std::vector<palm::Anchor> anchors;
    bool ready = false;
};

Nets& nets() {
    static Nets n;
    return n;
}
std::once_flag gLoadOnce;

struct Square {
    float side = 1.0f, ox = 0.0f, oy = 0.0f;
    int w = 1, h = 1;
    float pixelX(float u) const { return u * side - ox; }
    float pixelY(float v) const { return v * side - oy; }
    float frameX(float u) const { return pixelX(u) / (float) w; }
    float frameY(float v) const { return pixelY(v) / (float) h; }
};

Square squareFor(const juce::Image& img) {
    Square s;
    s.w = img.getWidth();
    s.h = img.getHeight();
    s.side = (float) std::max(s.w, s.h);
    s.ox = (s.side - (float) s.w) * 0.5f;
    s.oy = (s.side - (float) s.h) * 0.5f;
    return s;
}

void sampleCrop(const juce::Image& src, const Square& sq, float cx, float cy,
                float size, float angle, int side, std::vector<float>& out) {
    out.assign((size_t) side * side * 3, 0.0f);
    const juce::Image::BitmapData bd(src, juce::Image::BitmapData::readOnly);
    const float ca = std::cos(angle), sa = std::sin(angle);
    for (int y = 0; y < side; ++y)
        for (int x = 0; x < side; ++x) {
            const float u = ((float) x + 0.5f) / (float) side - 0.5f;
            const float v = ((float) y + 0.5f) / (float) side - 0.5f;
            const float rx = u * ca - v * sa, ry = u * sa + v * ca;
            const int px = (int) sq.pixelX(cx + rx * size);
            const int py = (int) sq.pixelY(cy + ry * size);
            if (px < 0 || px >= bd.width || py < 0 || py >= bd.height) continue;
            const juce::Colour c = bd.getPixelColour(px, py);
            float* p = out.data() + ((size_t) y * side + x) * 3;
            p[0] = (float) c.getRed() / 255.0f;
            p[1] = (float) c.getGreen() / 255.0f;
            p[2] = (float) c.getBlue() / 255.0f;
        }
}

}

bool handsPortableReady(const std::string& modelPath) {
    std::call_once(gLoadOnce, [&] {
        auto& n = nets();
        n.ready = HandNet::loadFile(modelPath, n.detector, n.landmarks);
        if (n.ready) n.anchors = palm::anchors();
    });
    return nets().ready;
}

bool handsBuiltInReady() {
    return handsPortableReady(
        resolveAssetRef("asset:Models/hands/hands.humnet").getFullPathName().toStdString());
}

int detectHandLandmarksPortable(const juce::Image& frame, bool mirror, float minConfidence,
                                HandLandmarks* out, int maxHands) {
    auto& n = nets();
    if (!n.ready || !frame.isValid()) return 0;

    palm::Config cfg;
    cfg.minScore = std::max(0.1f, minConfidence);

    const Square sq = squareFor(frame);
    std::vector<float> in;
    sampleCrop(frame, sq, 0.5f, 0.5f, 1.0f, 0.0f, cfg.inputSize, in);
    if (!n.detector.run(in.data(), (int) in.size())) return 0;

    const auto& boxes = n.detector.output(0);
    const auto& scores = n.detector.output(1);
    auto palms = palm::detections(boxes.data.data(), scores.data.data(), n.anchors,
                                  cfg, std::min(maxHands, 2));
    if (palms.empty()) return 0;

    int found = 0;
    for (const auto& p : palms) {
        if (found >= maxHands) break;
        const auto crop = palm::cropFor(p);
        sampleCrop(frame, sq, crop.cx, crop.cy, crop.size, crop.angle, 224, in);
        if (!n.landmarks.run(in.data(), (int) in.size())) continue;

        const float presence = n.landmarks.output(1).data[0];
        if (presence < minConfidence) continue;

        auto& h = out[found];
        h.present = true;
        h.confidence = presence;
        const auto& pts = n.landmarks.output(0).data;
        const float ca = std::cos(crop.angle), sa = std::sin(crop.angle);
        for (int k = 0; k < 21; ++k) {
            const float u = pts[(size_t) (k * 3)] / 224.0f - 0.5f;
            const float v = pts[(size_t) (k * 3 + 1)] / 224.0f - 0.5f;
            const float sx = crop.cx + (u * ca - v * sa) * crop.size;
            const float sy = crop.cy + (u * sa + v * ca) * crop.size;
            float x = sq.frameX(sx);
            if (mirror) x = 1.0f - x;
            h.pt[(size_t) k][0] = std::clamp(x, 0.0f, 1.0f);
            h.pt[(size_t) k][1] = std::clamp(sq.frameY(sy), 0.0f, 1.0f);
        }
        ++found;
    }
    return found;
}

}
