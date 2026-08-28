#pragma once
#include <string>
#include <vector>

namespace hum {

class EngineHost;
struct Pattern;
struct SnapshotPattern;

class MetaEditor {
public:
    explicit MetaEditor(EngineHost& host) : host_(host) {}

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

private:
    void capturePatterns(std::vector<SnapshotPattern>& out) const;
    void applyMorphedPatterns(double x, double y);
    void applyStoredPattern(const std::string& organism, const Pattern& pattern);
    EngineHost& host_;
};

}
