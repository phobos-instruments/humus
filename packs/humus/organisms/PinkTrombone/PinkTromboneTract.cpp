// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "PinkTrombone/PinkTrombone.h"

#include <algorithm>
#include <cmath>

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {

double moveTowards(double current, double target, double up, double down) {
    if (current < target) return std::min(current + up, target);
    return std::max(current - down, target);
}
}

void TromboneTract::init() {
    for (int i = 0; i < kN; ++i) {
        double d = 0.0;
        if (i < 7.0 * kN / 44.0 - 0.5) d = 0.6;
        else if (i < 12.0 * kN / 44.0) d = 1.1;
        else d = 1.5;
        diameter[(size_t) i] = restDiameter[(size_t) i] = targetDiameter[(size_t) i] = d;
    }
    R.fill(0.0);
    L.fill(0.0);
    junctionR.fill(0.0);
    junctionL.fill(0.0);
    reflection.fill(0.0);
    newReflection.fill(0.0);
    noseR.fill(0.0);
    noseL.fill(0.0);
    noseJunctionR.fill(0.0);
    noseJunctionL.fill(0.0);
    for (int i = 0; i < kNoseLength; ++i) {
        const double d = 2.0 * i / kNoseLength;
        double diam = d < 1.0 ? 0.4 + 1.6 * d : 0.5 + 1.5 * (2.0 - d);
        noseDiameter[(size_t) i] = std::min(diam, 1.9);
    }
    newReflectionLeft = newReflectionRight = newReflectionNose = 0.0;
    lastObstruction = -1;
    transientCount = 0;
    lipOutput = noseOutput = 0.0;
    velumTarget = 0.01;
    calculateReflections();
    calculateNoseReflections();
    noseDiameter[0] = velumTarget;
    reflectionLeft = newReflectionLeft;
    reflectionRight = newReflectionRight;
    reflectionNose = newReflectionNose;
    for (int i = 0; i <= kN; ++i) reflection[(size_t) i] = newReflection[(size_t) i];
}

void TromboneTract::setRestDiameter(double tongueIndex, double tongueDiameter) {
    for (int i = kBladeStart; i < kLipStart; ++i) {
        const double t =
            1.1 * kPi * (tongueIndex - i) / (double) (kTipStart - kBladeStart);
        const double fixedTongueDiameter = 2.0 + (tongueDiameter - 2.0) / 1.5;
        double curve = (1.5 - fixedTongueDiameter + 1.7) * std::cos(t);
        if (i == kBladeStart - 2 || i == kLipStart - 1) curve *= 0.8;
        if (i == kBladeStart || i == kLipStart - 2) curve *= 0.94;
        restDiameter[(size_t) i] = 1.5 - curve;
    }
    for (int i = 0; i < kN; ++i) targetDiameter[(size_t) i] = restDiameter[(size_t) i];
}

void TromboneTract::applyConstriction(double index, double diam) {
    diam -= 0.3;
    if (diam < 0.0) diam = 0.0;
    double width = 2.0;
    if (index < 25.0) width = 10.0;
    else if (index >= kTipStart) width = 5.0;
    else width = 10.0 - 5.0 * (index - 25.0) / (double) (kTipStart - 25);
    if (index < 2.0 || index >= kN || diam >= 3.0) return;
    const int intIndex = (int) std::lround(index);
    for (int i = -(int) std::ceil(width) - 1; i < (int) width + 1; ++i) {
        if (intIndex + i < 0 || intIndex + i >= kN) continue;
        double relpos = std::abs((intIndex + i) - index) - 0.5;
        double shrink;
        if (relpos <= 0.0) shrink = 0.0;
        else if (relpos > width) shrink = 1.0;
        else shrink = 0.5 * (1.0 - std::cos(kPi * relpos / width));
        auto& target = targetDiameter[(size_t) (intIndex + i)];
        if (diam < target) target = diam + (target - diam) * shrink;
    }
}

void TromboneTract::reshape(double deltaTime) {
    const double amount = deltaTime * 15.0;
    int newLastObstruction = -1;
    for (int i = 0; i < kN; ++i) {
        const double d = diameter[(size_t) i];
        const double target = targetDiameter[(size_t) i];
        if (d <= 0.0) newLastObstruction = i;
        double slowReturn;
        if (i < kNoseStart) slowReturn = 0.6;
        else if (i >= kTipStart) slowReturn = 1.0;
        else slowReturn = 0.6 + 0.4 * (i - kNoseStart) / (double) (kTipStart - kNoseStart);
        diameter[(size_t) i] =
            moveTowards(d, target, slowReturn * amount, 2.0 * amount);
    }
    if (lastObstruction > -1 && newLastObstruction == -1 && noseA[0] < 0.05
        && transientCount < kMaxTransients) {
        transients[(size_t) transientCount++] = {lastObstruction, 0.0};
    }
    lastObstruction = newLastObstruction;
    noseDiameter[0] =
        moveTowards(noseDiameter[0], velumTarget, amount * 0.25, amount * 0.1);
    noseA[0] = noseDiameter[0] * noseDiameter[0];
}

void TromboneTract::calculateReflections() {
    for (int i = 0; i < kN; ++i)
        A[(size_t) i] = diameter[(size_t) i] * diameter[(size_t) i];
    for (int i = 1; i < kN; ++i) {
        reflection[(size_t) i] = newReflection[(size_t) i];
        if (A[(size_t) i] == 0.0) newReflection[(size_t) i] = 0.999;
        else
            newReflection[(size_t) i] = (A[(size_t) (i - 1)] - A[(size_t) i])
                                      / (A[(size_t) (i - 1)] + A[(size_t) i]);
    }
    reflectionLeft = newReflectionLeft;
    reflectionRight = newReflectionRight;
    reflectionNose = newReflectionNose;
    const double sum = A[kNoseStart] + A[kNoseStart + 1] + noseA[0];
    newReflectionLeft = (2.0 * A[kNoseStart] - sum) / sum;
    newReflectionRight = (2.0 * A[kNoseStart + 1] - sum) / sum;
    newReflectionNose = (2.0 * noseA[0] - sum) / sum;
}

void TromboneTract::calculateNoseReflections() {
    for (int i = 0; i < kNoseLength; ++i)
        noseA[(size_t) i] = noseDiameter[(size_t) i] * noseDiameter[(size_t) i];
    for (int i = 1; i < kNoseLength; ++i)
        noseReflection[(size_t) i] =
            (noseA[(size_t) (i - 1)] - noseA[(size_t) i])
            / (noseA[(size_t) (i - 1)] + noseA[(size_t) i]);
}

void TromboneTract::runStep(double glottalOutput, double turbulence, double lambda,
                            double noiseMod, double constrIndex, double constrDiameter,
                            double sampleRate) {
    for (int t = 0; t < transientCount; ++t) {
        auto& trans = transients[(size_t) t];
        const double amplitude = 0.3 * std::pow(2.0, -200.0 * trans.timeAlive);
        R[(size_t) trans.position] += amplitude / 2.0;
        L[(size_t) trans.position] += amplitude / 2.0;
        trans.timeAlive += 1.0 / (sampleRate * 2.0);
    }
    for (int t = transientCount - 1; t >= 0; --t)
        if (transients[(size_t) t].timeAlive > 0.2)
            transients[(size_t) t] = transients[(size_t) --transientCount];

    if (constrDiameter > 0.0 && turbulence != 0.0) {
        const int i = (int) std::floor(constrIndex);
        const double delta = constrIndex - i;
        const double turb = turbulence * noiseMod;
        const double thinness = std::clamp(8.0 * (0.7 - constrDiameter), 0.0, 1.0);
        const double openness = std::clamp(30.0 * (constrDiameter - 0.3), 0.0, 1.0);
        const double noise0 = turb * (1.0 - delta) * thinness * openness;
        const double noise1 = turb * delta * thinness * openness;
        if (i + 1 >= 0 && i + 1 < kN) {
            R[(size_t) (i + 1)] += noise0 / 2.0;
            L[(size_t) (i + 1)] += noise0 / 2.0;
        }
        if (i + 2 >= 0 && i + 2 < kN) {
            R[(size_t) (i + 2)] += noise1 / 2.0;
            L[(size_t) (i + 2)] += noise1 / 2.0;
        }
    }

    junctionR[0] = L[0] * 0.75 + glottalOutput;
    junctionL[kN] = R[kN - 1] * -0.85;

    for (int i = 1; i < kN; ++i) {
        const double r =
            reflection[(size_t) i] * (1.0 - lambda) + newReflection[(size_t) i] * lambda;
        const double w = r * (R[(size_t) (i - 1)] + L[(size_t) i]);
        junctionR[(size_t) i] = R[(size_t) (i - 1)] - w;
        junctionL[(size_t) i] = L[(size_t) i] + w;
    }

    const int in = kNoseStart;
    double r = newReflectionLeft * (1.0 - lambda) + reflectionLeft * lambda;
    junctionL[(size_t) in] = r * R[(size_t) (in - 1)] + (1.0 + r) * (noseL[0] + L[(size_t) in]);
    r = newReflectionRight * (1.0 - lambda) + reflectionRight * lambda;
    junctionR[(size_t) in] = r * L[(size_t) in] + (1.0 + r) * (R[(size_t) (in - 1)] + noseL[0]);
    r = newReflectionNose * (1.0 - lambda) + reflectionNose * lambda;
    noseJunctionR[0] = r * noseL[0] + (1.0 + r) * (L[(size_t) in] + R[(size_t) (in - 1)]);

    for (int i = 0; i < kN; ++i) {
        R[(size_t) i] = junctionR[(size_t) i] * 0.999;
        L[(size_t) i] = junctionL[(size_t) (i + 1)] * 0.999;
    }
    lipOutput = R[kN - 1];

    noseJunctionL[kNoseLength] = noseR[kNoseLength - 1] * -0.85;
    for (int i = 1; i < kNoseLength; ++i) {
        const double w = noseReflection[(size_t) i] * (noseR[(size_t) (i - 1)] + noseL[(size_t) i]);
        noseJunctionR[(size_t) i] = noseR[(size_t) (i - 1)] - w;
        noseJunctionL[(size_t) i] = noseL[(size_t) i] + w;
    }
    for (int i = 0; i < kNoseLength; ++i) {
        noseR[(size_t) i] = noseJunctionR[(size_t) i];
        noseL[(size_t) i] = noseJunctionL[(size_t) (i + 1)];
    }
    noseOutput = noseR[kNoseLength - 1];
}

}
