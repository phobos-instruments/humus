#pragma once
#include <algorithm>
#include <array>
#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

struct BodyLandmarks {
    bool present = false;
    float confidence = 0.0f;
    std::array<std::array<float, 2>, 33> pt{};
    std::array<float, 33> vis{};
};

namespace bodypose {

constexpr int kNose = 0;
constexpr int kShoulderL = 11, kShoulderR = 12;
constexpr int kElbowL = 13, kElbowR = 14;
constexpr int kWristL = 15, kWristR = 16;
constexpr int kHipL = 23, kHipR = 24;
constexpr int kAnkleL = 27, kAnkleR = 28;

constexpr int kPerBody = 12;
constexpr int kFeatures = 8;

constexpr int kSigHeadX = 0, kSigHeadY = 1;
constexpr int kSigHandLX = 2, kSigHandLY = 3, kSigHandRX = 4, kSigHandRY = 5;
constexpr int kSigRaiseL = 6, kSigRaiseR = 7;
constexpr int kSigLean = 8, kSigCrouch = 9, kSigStance = 10;
constexpr int kSigPresent = 11;

struct BodyValues {
    std::array<float, kPerBody> sig{};
    std::array<bool, kPerBody> valid{};
};

inline float dist(const std::array<float, 2>& a, const std::array<float, 2>& b) {
    return std::hypot(a[0] - b[0], a[1] - b[1]);
}

inline bool seen(const BodyLandmarks& b, int i) { return b.vis[(size_t) i] > 0.35f; }

inline std::array<float, 2> mid(const std::array<float, 2>& a,
                                const std::array<float, 2>& b) {
    return {(a[0] + b[0]) * 0.5f, (a[1] + b[1]) * 0.5f};
}

inline float clamp01(float v) { return std::clamp(v, 0.0f, 1.0f); }

inline float jointAngle(const BodyLandmarks& b, int root, int pivot, int tip) {
    const float ax = b.pt[(size_t) root][0] - b.pt[(size_t) pivot][0];
    const float ay = b.pt[(size_t) root][1] - b.pt[(size_t) pivot][1];
    const float bx = b.pt[(size_t) tip][0] - b.pt[(size_t) pivot][0];
    const float by = b.pt[(size_t) tip][1] - b.pt[(size_t) pivot][1];
    const float la = std::hypot(ax, ay), lb = std::hypot(bx, by);
    if (la < 1e-5f || lb < 1e-5f) return 0.0f;
    const float c = std::clamp((ax * bx + ay * by) / (la * lb), -1.0f, 1.0f);
    return std::acos(c) / kPiF;
}

struct Torso {
    bool ok = false;
    std::array<float, 2> shoulderMid{}, hipMid{};
    float length = 0.0f;
};

inline Torso torsoOf(const BodyLandmarks& b) {
    Torso t;
    if (!seen(b, kShoulderL) || !seen(b, kShoulderR) || !seen(b, kHipL)
        || !seen(b, kHipR))
        return t;
    t.shoulderMid = mid(b.pt[kShoulderL], b.pt[kShoulderR]);
    t.hipMid = mid(b.pt[kHipL], b.pt[kHipR]);
    t.length = dist(t.shoulderMid, t.hipMid);
    t.ok = t.length > 1e-3f;
    return t;
}

inline float raiseOf(const BodyLandmarks& b, const Torso& t, int shoulder, int wrist) {
    return clamp01(0.5f
                   + (b.pt[(size_t) shoulder][1] - b.pt[(size_t) wrist][1])
                         / (2.0f * t.length));
}

inline BodyValues values(const BodyLandmarks& b) {
    BodyValues v;
    if (!b.present) return v;
    v.sig[kSigPresent] = 1.0f;
    v.valid[kSigPresent] = true;

    if (seen(b, kNose)) {
        v.sig[kSigHeadX] = clamp01(b.pt[kNose][0]);
        v.sig[kSigHeadY] = clamp01(1.0f - b.pt[kNose][1]);
        v.valid[kSigHeadX] = v.valid[kSigHeadY] = true;
    }
    if (seen(b, kWristL)) {
        v.sig[kSigHandLX] = clamp01(b.pt[kWristL][0]);
        v.sig[kSigHandLY] = clamp01(1.0f - b.pt[kWristL][1]);
        v.valid[kSigHandLX] = v.valid[kSigHandLY] = true;
    }
    if (seen(b, kWristR)) {
        v.sig[kSigHandRX] = clamp01(b.pt[kWristR][0]);
        v.sig[kSigHandRY] = clamp01(1.0f - b.pt[kWristR][1]);
        v.valid[kSigHandRX] = v.valid[kSigHandRY] = true;
    }

    const Torso t = torsoOf(b);
    if (!t.ok) return v;

    if (seen(b, kWristL)) {
        v.sig[kSigRaiseL] = raiseOf(b, t, kShoulderL, kWristL);
        v.valid[kSigRaiseL] = true;
    }
    if (seen(b, kWristR)) {
        v.sig[kSigRaiseR] = raiseOf(b, t, kShoulderR, kWristR);
        v.valid[kSigRaiseR] = true;
    }
    v.sig[kSigLean] = clamp01(0.5f + (t.shoulderMid[0] - t.hipMid[0]) / t.length);
    v.valid[kSigLean] = true;

    if (seen(b, kAnkleL) && seen(b, kAnkleR)) {
        const auto ankleMid = mid(b.pt[kAnkleL], b.pt[kAnkleR]);
        const float legs = dist(t.hipMid, ankleMid);
        v.sig[kSigCrouch] = clamp01(1.0f - legs / (1.8f * t.length));
        v.valid[kSigCrouch] = true;
        const float shoulderW = dist(b.pt[kShoulderL], b.pt[kShoulderR]);
        if (shoulderW > 1e-3f) {
            const float ratio = dist(b.pt[kAnkleL], b.pt[kAnkleR]) / shoulderW;
            v.sig[kSigStance] = clamp01((ratio - 0.6f) / 2.4f);
            v.valid[kSigStance] = true;
        }
    }
    return v;
}

inline bool features(const BodyLandmarks& b, float out[kFeatures]) {
    for (int i = 0; i < kFeatures; ++i) out[i] = 0.0f;
    if (!b.present) return false;
    const Torso t = torsoOf(b);
    if (!t.ok || !seen(b, kWristL) || !seen(b, kWristR) || !seen(b, kElbowL)
        || !seen(b, kElbowR))
        return false;
    out[0] = raiseOf(b, t, kShoulderL, kWristL);
    out[1] = raiseOf(b, t, kShoulderR, kWristR);
    out[2] = clamp01(std::abs(b.pt[kWristL][0] - b.pt[kShoulderL][0]) / (1.6f * t.length));
    out[3] = clamp01(std::abs(b.pt[kWristR][0] - b.pt[kShoulderR][0]) / (1.6f * t.length));
    out[4] = jointAngle(b, kShoulderL, kElbowL, kWristL);
    out[5] = jointAngle(b, kShoulderR, kElbowR, kWristR);
    out[6] = clamp01(0.5f + (t.shoulderMid[0] - t.hipMid[0]) / t.length);
    if (seen(b, kAnkleL) && seen(b, kAnkleR)) {
        const auto ankleMid = mid(b.pt[kAnkleL], b.pt[kAnkleR]);
        out[7] = clamp01(1.0f - dist(t.hipMid, ankleMid) / (1.8f * t.length));
    }
    return true;
}

inline const int (*bones(int& count))[2] {
    static const int kBones[][2] = {
        {0, 1},   {1, 2},   {2, 3},   {3, 7},
        {0, 4},   {4, 5},   {5, 6},   {6, 8},
        {9, 10},
        {11, 12}, {11, 13}, {13, 15}, {15, 17}, {15, 19}, {15, 21}, {17, 19},
        {12, 14}, {14, 16}, {16, 18}, {16, 20}, {16, 22}, {18, 20},
        {11, 23}, {12, 24}, {23, 24},
        {23, 25}, {25, 27}, {27, 29}, {27, 31}, {29, 31},
        {24, 26}, {26, 28}, {28, 30}, {28, 32}, {30, 32},
    };
    count = (int) (sizeof(kBones) / sizeof(kBones[0]));
    return kBones;
}

}
}
