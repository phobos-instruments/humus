#include "Skeleton/BodyPortable.h"

#include "Skeleton/BodyCoreML.h"

#include <algorithm>
#include <cmath>
#include <mutex>
#include <vector>

#include "core/AppPaths.h"

#include "Skeleton/PoseDecode.h"
#include "common/FrameSample.h"
#include "common/HumNet.h"
#include "common/SsdDecode.h"

namespace hum {
namespace {

using framesample::sampleCrop;
using framesample::squareFor;

struct Nets {
    HumNet detector, landmarks;
    std::vector<ssd::Anchor> anchors = ssd::anchors(posedecode::config());
    juce::CriticalSection lock;
    bool ready = false;
};

Nets& nets() {
    static Nets n;
    return n;
}
std::once_flag gLoadOnce;

float sigmoid(float v) { return 1.0f / (1.0f + std::exp(-v)); }

}

bool bodyPortableReady(const std::string& modelPath) {
    std::call_once(gLoadOnce, [&] {
        auto& n = nets();
        n.ready = HumNet::loadFile(modelPath, n.detector, n.landmarks);
    });
    return nets().ready;
}

bool bodyBuiltInReady() {
#if JUCE_MAC
    if (bodyCoreMLReady()) return true;
#endif
    return bodyPortableReady(
        resolveAssetRef("asset:Models/body/body.humnet").getFullPathName().toStdString());
}

bool detectBodyLandmarksPortable(const juce::Image& frame, bool mirror,
                                 float minConfidence, BodyLandmarks& out,
                                 BodyTrack& track) {
    out = BodyLandmarks{};
    auto& n = nets();
    if (!frame.isValid()) return false;
#if JUCE_MAC
    if (!n.ready && !bodyCoreMLReady()) return false;
#else
    if (!n.ready) return false;
#endif
    const juce::ScopedLock sl(n.lock);

    const auto cfg = posedecode::config();
    const auto sq = squareFor(frame);
    std::vector<float> in;

    ssd::Crop crop{};
    if (track.tracking) {
        crop = posedecode::cropFromAlignment(track.cx, track.cy, track.ax, track.ay);
    } else {
        auto det = cfg;
        det.minScore = std::max(0.1f, minConfidence);
        sampleCrop(frame, sq, 0.5f, 0.5f, 1.0f, 0.0f, det.inputSize, in);
        const float* boxData = nullptr;
        const float* scoreData = nullptr;
#if JUCE_MAC
        static thread_local std::vector<float> mlBoxes, mlScores;
        if (bodyCoreMLPredict(false, in.data(), (int) in.size(), mlBoxes, mlScores)) {
            boxData = mlBoxes.data();
            scoreData = mlScores.data();
        }
#endif
        if (boxData == nullptr) {
            if (!n.detector.run(in.data(), (int) in.size())) return false;
            boxData = n.detector.output(0).data.data();
            scoreData = n.detector.output(1).data.data();
        }
        const auto hits = ssd::detections(boxData, scoreData, n.anchors, det, 1);
        if (hits.empty()) return false;
        crop = posedecode::cropFor(hits[0]);
    }
    if (crop.size < 1e-3f) {
        track.tracking = false;
        return false;
    }

    sampleCrop(frame, sq, crop.cx, crop.cy, crop.size, crop.angle, 256, in);
    const float* ptData = nullptr;
    float presence = 0.0f;
#if JUCE_MAC
    static thread_local std::vector<float> mlPts, mlPresence;
    if (bodyCoreMLPredict(true, in.data(), (int) in.size(), mlPts, mlPresence)
        && !mlPresence.empty()) {
        ptData = mlPts.data();
        presence = mlPresence[0];
    }
#endif
    if (ptData == nullptr) {
        if (!n.landmarks.run(in.data(), (int) in.size())) {
            track.tracking = false;
            return false;
        }
        ptData = n.landmarks.output(0).data.data();
        presence = n.landmarks.output(1).data[0];
    }
    if (presence < std::max(0.1f, minConfidence)) {
        track.tracking = false;
        return false;
    }

    const float* pts = ptData;
    const float ca = std::cos(crop.angle), sa = std::sin(crop.angle);
    auto toSquare = [&](int k, float& sx, float& sy) {
        const float u = pts[(size_t) (k * 5)] / 256.0f - 0.5f;
        const float v = pts[(size_t) (k * 5 + 1)] / 256.0f - 0.5f;
        sx = crop.cx + (u * ca - v * sa) * crop.size;
        sy = crop.cy + (u * sa + v * ca) * crop.size;
    };

    for (int k = 0; k < 33; ++k) {
        float sx, sy;
        toSquare(k, sx, sy);
        float x = sq.frameX(sx);
        if (mirror) x = 1.0f - x;
        out.pt[(size_t) k][0] = std::clamp(x, 0.0f, 1.0f);
        out.pt[(size_t) k][1] = std::clamp(sq.frameY(sy), 0.0f, 1.0f);
        out.vis[(size_t) k] = sigmoid(pts[(size_t) (k * 5 + 3)]);
    }
    out.present = true;
    out.confidence = presence;

    toSquare(33, track.cx, track.cy);
    toSquare(34, track.ax, track.ay);
    track.tracking = true;
    return true;
}

}
