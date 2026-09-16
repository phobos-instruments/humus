// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include "gui/host/BrickHost.h"
#include "gui/host/HostCore.h"
#include "io/MeterMap.h"
#include "io/PatchDocument.h"

namespace hum {


class AutomationHost {
public:
    AutomationHost(BrickHost& host, HostCore& core) : host_(host), doc_(core), audio_(core), nodes_(core), capture_(core) {}

    bool tempoAutomated() const;
    void setTempoAutomated(bool on);
    bool meterAutomated() const;
    void setMeterAutomated(bool on);

    bool isAutomated(const std::string& organism, const std::string& param) const;
    void add(const std::string& organism, const std::string& param);
    void remove(const std::string& organism, const std::string& param);
    void addPoint(const std::string& organism, const std::string& param, double beat, double value);
    int  movePoint(const std::string& organism, const std::string& param,
                   int index, double beat, double value);
    void deletePoint(const std::string& organism, const std::string& param, int index);
    void setPoints(const std::string& organism, const std::string& param,
                   const std::vector<AutomationBreakpoint>& points);
    void clearLane(const std::string& organism, const std::string& param,
                   double from = 0.0, double to = -1.0);
    void clearOrganism(const std::string& organism, bool deleteLanes);
    void setLaneMute(const std::string& organism, const std::string& param, bool mute);
    std::string laneKind(const std::string& organism, const std::string& param) const;
    void addRangePoint(const std::string& organism, const std::string& param,
                       double beat, double lo, double hi);
    int  moveRangePoint(const std::string& organism, const std::string& param,
                        int index, double beat, double lo, double hi);
    void addTriggerPoint(const std::string& organism, const std::string& param, double beat);
    void setCurve(const std::string& organism, const std::string& param,
                  int index, double curve);
    double curveAt(const std::string& organism, const std::string& param, int index) const;

    const std::vector<PerformanceBox>& boxes() const;
    void moveBox(int index, double deltaBeats);
    void trimBox(int index, double newStart, double newEnd);
    int splitBox(int index, double atBeat);
    int duplicateBox(int index, double atBeat);
    void deleteBox(int index);

    bool isHeld(const std::string& organism, const std::string& param) const;
    bool anyHeld(const std::string& organism) const;
    void release(const std::string& organism, const std::string& param);
    void releaseAll();

    void setMasterRecord(bool on);
    bool isMasterRecord() const;

    void   setLoop(double startBeat, double endBeat, bool enabled);
    bool   loopEnabled() const;
    double loopStartBeat() const;
    double loopEndBeat() const;
    void   setTimeSignature(int numerator, int denominator);
    int    timeSigNumerator() const;
    Meter  timeSig() const;
    MeterMap meterMap() const;
    Meter  meterAt(double beat) const { return meterMap().at(beat); }

    void clearTimeRange(double from, double to);
    void deleteTimeRange(double from, double to);
    void insertTime(double at, double amount);
    void copyTimeRange(double from, double to);
    void cutTimeRange(double from, double to);
    bool pasteTimeRange(double at);
    bool hasClip() const { return hasAutoClip_; }

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
