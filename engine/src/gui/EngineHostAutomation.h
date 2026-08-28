#pragma once
#include <string>
#include <vector>

#include "io/PatchDocument.h"

namespace hum {

class EngineHost;

class AutomationHost {
public:
    explicit AutomationHost(EngineHost& host) : host_(host) {}

    bool tempoAutomated() const;
    void setTempoAutomated(bool on);

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

    void clearTimeRange(double from, double to);
    void deleteTimeRange(double from, double to);
    void insertTime(double at, double amount);
    void copyTimeRange(double from, double to);
    void cutTimeRange(double from, double to);
    bool pasteTimeRange(double at);
    bool hasClip() const { return hasAutoClip_; }

private:
    EngineHost& host_;
    struct AutoClipLane {
        std::string organism, param, kind;
        std::vector<AutomationBreakpoint> points;
    };
    static std::vector<AutoClipLane> autoClip_;
    static double autoClipSpan_;
    static bool hasAutoClip_;
};

}
