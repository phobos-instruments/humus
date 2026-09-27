// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include "gui/host/AutomationEdits.h"
#include "gui/host/BrickHost.h"
#include "gui/host/HostCore.h"
#include "io/MeterMap.h"
#include "io/PatchDocument.h"

namespace hum {


class AutomationHost : public AutomationEdits {
public:
    AutomationHost(BrickHost& host, HostCore& core) : host_(host), doc_(core), audio_(core), nodes_(core), capture_(core) {}

    bool tempoAutomated() const override;
    void setTempoAutomated(bool on) override;
    bool meterAutomated() const override;
    void setMeterAutomated(bool on) override;

    bool isAutomated(const std::string& organism, const std::string& param) const override;
    void add(const std::string& organism, const std::string& param) override;
    void remove(const std::string& organism, const std::string& param) override;
    void addPoint(const std::string& organism, const std::string& param, double beat, double value) override;
    int  movePoint(const std::string& organism, const std::string& param,
                   int index, double beat, double value) override;
    void deletePoint(const std::string& organism, const std::string& param, int index) override;
    void setPoints(const std::string& organism, const std::string& param,
                   const std::vector<AutomationBreakpoint>& points) override;
    void clearLane(const std::string& organism, const std::string& param,
                   double from = 0.0, double to = -1.0) override;
    void clearOrganism(const std::string& organism, bool deleteLanes) override;
    void setLaneMute(const std::string& organism, const std::string& param, bool mute) override;
    std::string laneKind(const std::string& organism, const std::string& param) const override;
    void addRangePoint(const std::string& organism, const std::string& param,
                       double beat, double lo, double hi) override;
    int  moveRangePoint(const std::string& organism, const std::string& param,
                        int index, double beat, double lo, double hi) override;
    void addTriggerPoint(const std::string& organism, const std::string& param, double beat) override;
    void setCurve(const std::string& organism, const std::string& param,
                  int index, double curve) override;
    double curveAt(const std::string& organism, const std::string& param, int index) const override;

    const std::vector<PerformanceBox>& boxes() const override;
    void moveBox(int index, double deltaBeats) override;
    void trimBox(int index, double newStart, double newEnd) override;
    int splitBox(int index, double atBeat) override;
    int duplicateBox(int index, double atBeat) override;
    void deleteBox(int index) override;

    bool isHeld(const std::string& organism, const std::string& param) const override;
    bool anyHeld(const std::string& organism) const override;
    void release(const std::string& organism, const std::string& param) override;
    void releaseAll() override;

    void setMasterRecord(bool on) override;
    bool isMasterRecord() const override;

    void   setLoop(double startBeat, double endBeat, bool enabled) override;
    bool   loopEnabled() const override;
    double loopStartBeat() const override;
    double loopEndBeat() const override;
    void   setTimeSignature(int numerator, int denominator) override;
    int    timeSigNumerator() const override;
    Meter  timeSig() const override;
    MeterMap meterMap() const override;
    Meter  meterAt(double beat) const override { return meterMap().at(beat); }

    void clearTimeRange(double from, double to) override;
    void deleteTimeRange(double from, double to) override;
    void insertTime(double at, double amount) override;
    void copyTimeRange(double from, double to) override;
    void cutTimeRange(double from, double to) override;
    bool pasteTimeRange(double at) override;
    bool hasClip() const override { return hasAutoClip_; }

private:
    bool capturingLive() const;

    BrickHost& host_;
    HostDocument& doc_;
    HostGraph& audio_;
    HostNodes& nodes_;
    HostCapture& capture_;
    bool masterRecord_ = false;
    struct AutoClipLane {
        std::string organism, param, kind;
        std::vector<AutomationBreakpoint> points;
    };
    static std::vector<AutoClipLane> autoClip_;
    static double autoClipSpan_;
    static bool hasAutoClip_;
};

}
