// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include "gui/host/BrickHost.h"
#include "gui/host/HostCore.h"

namespace hum {

struct Pattern;
struct SnapshotPattern;

class MetaEditor {
public:
    MetaEditor(BrickHost& host, HostCore& core) : host_(host), doc_(core), patternSync_(core), nodes_(core), capture_(core) {}

    void apply(double x, double y);
    void morph(double x, double y);
    void applyPathAt(double beat);
    void replayRecallAt(double beat);
    bool hasMorphPath() const;
    int  addSnapshot(const std::string& name);
    void storeSnapshot(int index);
    void recallSnapshot(int index);
    void clearSnapshot(int index);
    void renameSnapshot(int index, const std::string& name);
    void setSnapshotColour(int index, const std::string& hexColour);
    void placeSnapshot(int index, double x, double y);
    void movePoint(int pointIndex, double x, double y);
    void removePoint(int pointIndex);
    void setMask(const std::string& organism, int propertyIndex, bool restore);
    void setTemperature(double t);
    void applyTarget(const std::string& param, double value);
    double x() const { return metaX_; }
    double y() const { return metaY_; }

private:
    void capturePatterns(std::vector<SnapshotPattern>& out) const;
    void applyMorphedPatterns(double x, double y);
    void applyStoredPattern(const std::string& organism, const Pattern& pattern);
    BrickHost& host_;
    HostDocument& doc_;
    HostPatterns& patternSync_;
    HostNodes& nodes_;
    HostCapture& capture_;
    double metaX_ = 0.5, metaY_ = 0.5;
    bool metaSnapArmed_ = false;
    bool metaHoldApply_ = false;
    int metaLastRecall_ = -1;
    double metaRecallBeat_ = 0.0;
};

}
