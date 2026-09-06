#include "Hands/HandsPortable.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <mutex>

#include "core/AppPaths.h"

#include "common/CoreMLNet.h"

#include "common/FrameSample.h"
#include "common/HumNet.h"
#include "common/SsdDecode.h"

namespace hum {
namespace {

using framesample::sampleCrop;
using framesample::squareFor;

struct Nets {
    HumNet detector, landmarks;
    std::vector<ssd::Anchor> anchors = ssd::anchors();
    bool ready = false;
};

Nets& nets() {
    static Nets n;
    return n;
}
std::once_flag gLoadOnce;

#if JUCE_MAC
struct MlNets {
    std::unique_ptr<CoreMLNet> det, lm;
    bool tried = false;
};

MlNets& mlNets() {
    static MlNets m;
    return m;
}

bool handsCoreMLReady() {
    auto& m = mlNets();
    if (!m.tried) {
        m.tried = true;
        m.det = CoreMLNet::load(resolveAssetRef("asset:Models/hands/det.mlpackage"));
        m.lm = CoreMLNet::load(resolveAssetRef("asset:Models/hands/lm.mlpackage"));
        if (m.det == nullptr || m.lm == nullptr) {
            m.det.reset();
            m.lm.reset();
        }
    }
    return m.det != nullptr;
}
#else
bool handsCoreMLReady() { return false; }
#endif

}

bool handsPortableReady(const std::string& modelPath) {
    std::call_once(gLoadOnce, [&] {
        auto& n = nets();
        n.ready = HumNet::loadFile(modelPath, n.detector, n.landmarks);
    });
    return nets().ready;
}

bool handsBuiltInReady() {
    if (handsCoreMLReady()) return true;
    return handsPortableReady(
        resolveAssetRef("asset:Models/hands/hands.humnet").getFullPathName().toStdString());
}

int detectHandLandmarksPortable(const juce::Image& frame, bool mirror, float minConfidence,
                                HandLandmarks* out, int maxHands) {
    auto& n = nets();
    if (!frame.isValid() || (!n.ready && !handsCoreMLReady())) return 0;

    ssd::Config cfg;
    cfg.minScore = std::max(0.1f, minConfidence);

    const auto sq = squareFor(frame);
    std::vector<float> in;
    sampleCrop(frame, sq, 0.5f, 0.5f, 1.0f, 0.0f, cfg.inputSize, in);
#if JUCE_MAC
    CoreMLNet* mlDet = handsCoreMLReady() ? mlNets().det.get() : nullptr;
    CoreMLNet* mlLm = handsCoreMLReady() ? mlNets().lm.get() : nullptr;
    static thread_local std::vector<std::vector<float>> mlOuts;
#endif
    const float* boxData = nullptr;
    const float* scoreData = nullptr;
#if JUCE_MAC
    if (mlDet != nullptr
        && mlDet->run(in.data(), (int) in.size(), cfg.inputSize, mlOuts, 2)) {
        boxData = mlOuts[0].data();
        scoreData = mlOuts[1].data();
    }
#endif
    if (boxData == nullptr) {
        if (!n.detector.run(in.data(), (int) in.size())) return 0;
        boxData = n.detector.output(0).data.data();
        scoreData = n.detector.output(1).data.data();
    }
    auto palms = ssd::detections(boxData, scoreData, n.anchors,
                                 cfg, std::min(maxHands, 2));
    if (palms.empty()) return 0;

    int found = 0;
    for (const auto& p : palms) {
        if (found >= maxHands) break;
        const auto crop = ssd::cropFor(p);
        sampleCrop(frame, sq, crop.cx, crop.cy, crop.size, crop.angle, 224, in);
        const float* ptData = nullptr;
        float presence = 0.0f;
#if JUCE_MAC
        static thread_local std::vector<std::vector<float>> lmOuts;
        if (mlLm != nullptr && mlLm->run(in.data(), (int) in.size(), 224, lmOuts, 2)
            && !lmOuts[1].empty()) {
            ptData = lmOuts[0].data();
            presence = lmOuts[1][0];
        }
#endif
        if (ptData == nullptr) {
            if (!n.landmarks.run(in.data(), (int) in.size())) continue;
            ptData = n.landmarks.output(0).data.data();
            presence = n.landmarks.output(1).data[0];
        }
        if (presence < minConfidence) continue;

        auto& h = out[found];
        h.present = true;
        h.confidence = presence;
        const float* pts = ptData;
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
