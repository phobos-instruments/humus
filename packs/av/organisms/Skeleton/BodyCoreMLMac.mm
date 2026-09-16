// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "Skeleton/BodyCoreML.h"

#include <memory>

#include "core/app/AppPaths.h"

#include "common/CoreMLNet.h"

namespace hum {

namespace {

struct Engine {
    std::unique_ptr<CoreMLNet> det, lm;
    bool tried = false;
};

Engine& engine() {
    static Engine e;
    return e;
}

}

bool bodyCoreMLReady() {
    auto& e = engine();
    if (!e.tried) {
        e.tried = true;
        e.det = CoreMLNet::load(resolveAssetRef("asset:Models/body/det.mlpackage"));
        e.lm = CoreMLNet::load(resolveAssetRef("asset:Models/body/lm.mlpackage"));
        if (e.det == nullptr || e.lm == nullptr) {
            e.det.reset();
            e.lm.reset();
        }
    }
    return e.det != nullptr;
}

bool bodyCoreMLPredict(bool landmarkNet, const float* in, int count,
                       std::vector<float>& out0, std::vector<float>& out1) {
    if (!bodyCoreMLReady()) return false;
    auto& net = landmarkNet ? engine().lm : engine().det;
    static thread_local std::vector<std::vector<float>> outs;
    if (!net->run(in, count, landmarkNet ? 256 : 224, outs, 2)) return false;
    out0 = std::move(outs[0]);
    out1 = std::move(outs[1]);
    return true;
}

}   // namespace hum
