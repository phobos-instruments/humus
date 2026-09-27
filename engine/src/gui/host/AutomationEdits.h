// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include "io/MeterMap.h"
#include "io/PatchDocument.h"

namespace hum {

class AutomationEdits {
public:
    virtual ~AutomationEdits() = default;

    virtual bool tempoAutomated() const = 0;
    virtual void setTempoAutomated(bool on) = 0;
    virtual bool meterAutomated() const = 0;
    virtual void setMeterAutomated(bool on) = 0;
    virtual bool isAutomated(const std::string& organism, const std::string& param) const = 0;
    virtual void add(const std::string& organism, const std::string& param) = 0;
    virtual void remove(const std::string& organism, const std::string& param) = 0;
    virtual void addPoint(const std::string& organism, const std::string& param, double beat, double value) = 0;
    virtual int movePoint(const std::string& organism, const std::string& param, int index, double beat, double value) = 0;
    virtual void deletePoint(const std::string& organism, const std::string& param, int index) = 0;
    virtual void setPoints(const std::string& organism, const std::string& param,
                           const std::vector<AutomationBreakpoint>& points) = 0;
    virtual void clearLane(const std::string& organism, const std::string& param, double from = 0.0, double to = -1.0) = 0;
    virtual void clearOrganism(const std::string& organism, bool deleteLanes) = 0;
    virtual void setLaneMute(const std::string& organism, const std::string& param, bool mute) = 0;
    virtual std::string laneKind(const std::string& organism, const std::string& param) const = 0;
    virtual void addRangePoint(const std::string& organism, const std::string& param, double beat, double lo,
                               double hi) = 0;
    virtual int moveRangePoint(const std::string& organism, const std::string& param, int index, double beat,
                               double lo, double hi) = 0;
    virtual void addTriggerPoint(const std::string& organism, const std::string& param, double beat) = 0;
    virtual void setCurve(const std::string& organism, const std::string& param, int index, double curve) = 0;
    virtual double curveAt(const std::string& organism, const std::string& param, int index) const = 0;
    virtual const std::vector<PerformanceBox>& boxes() const = 0;
    virtual void moveBox(int index, double deltaBeats) = 0;
    virtual void trimBox(int index, double newStart, double newEnd) = 0;
    virtual int splitBox(int index, double atBeat) = 0;
    virtual int duplicateBox(int index, double atBeat) = 0;
    virtual void deleteBox(int index) = 0;
    virtual bool isHeld(const std::string& organism, const std::string& param) const = 0;
    virtual bool anyHeld(const std::string& organism) const = 0;
    virtual void release(const std::string& organism, const std::string& param) = 0;
    virtual void releaseAll() = 0;
    virtual void setMasterRecord(bool on) = 0;
    virtual bool isMasterRecord() const = 0;
    virtual void setLoop(double startBeat, double endBeat, bool enabled) = 0;
    virtual bool loopEnabled() const = 0;
    virtual double loopStartBeat() const = 0;
    virtual double loopEndBeat() const = 0;
    virtual void setTimeSignature(int numerator, int denominator) = 0;
    virtual int timeSigNumerator() const = 0;
    virtual Meter timeSig() const = 0;
    virtual MeterMap meterMap() const = 0;
    virtual Meter meterAt(double beat) const = 0;
    virtual void clearTimeRange(double from, double to) = 0;
    virtual void deleteTimeRange(double from, double to) = 0;
    virtual void insertTime(double at, double amount) = 0;
    virtual void copyTimeRange(double from, double to) = 0;
    virtual void cutTimeRange(double from, double to) = 0;
    virtual bool pasteTimeRange(double at) = 0;
    virtual bool hasClip() const = 0;
};

}
