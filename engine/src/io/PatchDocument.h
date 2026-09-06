#pragma once
#include <cctype>
#include <cstdlib>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "hum/Parameter.h"
#include "hum/Pattern.h"

namespace juce { class XmlElement; }

namespace hum {

inline constexpr const char* kPatchRootTag = "humus-document";

inline bool isPatchRootTag(const std::string& tag) {
    static const std::string kSuffix = "-document";
    if (tag == "document") return true;
    return tag.size() > kSuffix.size() &&
           tag.compare(tag.size() - kSuffix.size(), kSuffix.size(), kSuffix) == 0;
}

struct PresetModel {
    int number = 0;
    std::string name;
    std::vector<Parameter> properties;
};

struct AutomationBreakpoint {
    double beat = 0.0;
    double value = 0.0;
    double valueMax = 0.0;
    double curve = 0.0;
};

struct AutomationLane {
    std::string propertyName;
    int propertyIndex = -1;
    std::string kind = "double";
    bool mute = false;
    bool record = false;
    std::vector<AutomationBreakpoint> points;
};

struct MidiControllerSource {
    std::string propertyName;
    int propertyIndex = -1;
    int cc = 0;
    std::vector<int> held;
    int port = 0;
    int channel = 0;
    std::string specType = "7-bit-control-change";
    double mapMin = 0.0;
    double mapMax = 1.0;
    double smoothing = 0.0;
    std::vector<std::pair<double, double>> curve;
    bool isSwitch = false;
    bool inverted = false;
    bool toggle = false;
    double threshold = 0.5;
};

struct OscControllerSource {
    std::string propertyName;
    int propertyIndex = -1;
    std::string address;
    double mapMin = 0.0;
    double mapMax = 1.0;
    double smoothing = 0.0;
    std::vector<std::pair<double, double>> curve;
    bool isSwitch = false;
    bool inverted = false;
    bool toggle = false;
    double threshold = 0.5;
};

struct ModControllerSource {
    std::string propertyName;
    int propertyIndex = -1;
    std::string sourceOrganism;
    std::string sourceValue;
    double mapMin = 0.0;
    double mapMax = 1.0;
    double smoothing = 0.0;
    std::vector<std::pair<double, double>> curve;
    bool isSwitch = false;
    bool inverted = false;
    bool toggle = false;
    double threshold = 0.5;
};

class SharedString {
public:
    SharedString() = default;
    SharedString& operator=(std::string s) {
        p_ = std::make_shared<const std::string>(std::move(s));
        return *this;
    }
    bool empty() const { return p_ == nullptr || p_->empty(); }
    const std::string& str() const {
        static const std::string kEmpty;
        return p_ != nullptr ? *p_ : kEmpty;
    }
    operator const std::string&() const { return str(); }
    bool operator==(const SharedString& o) const { return str() == o.str(); }
    bool operator!=(const SharedString& o) const { return !(*this == o); }

private:
    std::shared_ptr<const std::string> p_;
};

struct OrganismModel {
    std::string name;
    std::string classRaw;
    std::string displayClass;
    std::string kind;
    std::vector<Parameter> properties;
    bool hasBlob = false;
    SharedString pluginState;
    std::vector<PresetModel> presets;
    int currentPreset = 0;
    bool presetDirty = false;
    std::vector<AutomationLane> automation;
    std::vector<MidiControllerSource> midiSources;
    std::vector<OscControllerSource> oscSources;
    std::vector<ModControllerSource> modSources;
    Pattern pattern;
    bool internal = false;
    enum MidiReceive { kMidiOmni = 0, kMidiCordsOnly = 1, kMidiChannel = 2 };
    int midiReceiveMode = kMidiCordsOnly;
    int midiReceiveChannel = 1;
    int midiReceivePort = 0;
    std::set<std::string> rollLocked;

    std::string currentPresetName;
    std::string currentPresetSource;
};

inline constexpr const char* kBypassParam = "Bypass";

inline constexpr const char* kTrackMuteParam = "TrackMute";

inline constexpr const char* kSoloParam = "Solo";
inline constexpr const char* kArmParam = "Arm";

inline constexpr const char* kRandomAction = "Random";

inline constexpr const char* kPresetNextAction = "Preset Next";
inline constexpr const char* kPresetPrevAction = "Preset Prev";

inline bool isPresetStepAction(const std::string& param) {
    return param == kPresetNextAction || param == kPresetPrevAction;
}

inline constexpr const char* kPlayAction = "Play";
inline constexpr const char* kStopAction = "Stop";
inline constexpr const char* kPlayFromStartAction = "Play From Start";
inline constexpr const char* kGoToStartAction = "Go To Start";
inline constexpr const char* kGoToEndAction = "Go To End";
inline constexpr const char* kCaptureAction = "Record";
inline constexpr const char* kLoopToggleAction = "Loop";

inline bool isTransportAction(const std::string& param) {
    return param == kPlayAction || param == kStopAction
           || param == kPlayFromStartAction || param == kGoToStartAction
           || param == kGoToEndAction || param == kCaptureAction
           || param == kLoopToggleAction;
}

inline bool isClockPseudo(const std::string& displayClass) {
    return displayClass == "ClockPseudoSP";
}

inline constexpr const char* kTempoParam = "Tempo";
inline constexpr double kTempoMin = 20.0;
inline constexpr double kTempoMax = 999.0;
inline constexpr double kTempoLaneMin = 40.0;
inline constexpr double kTempoLaneMax = 300.0;

inline bool isMetapadPseudo(const std::string& displayClass) {
    return displayClass == "MetasurfacePseudoSP";
}
inline constexpr const char* kMetaXParam = "X";
inline constexpr const char* kMetaYParam = "Y";
inline constexpr const char* kMetaInterpolateParam = "Interpolate";
inline constexpr const char* kMetaSnapshotAction = "Snapshot";
inline constexpr const char* kMetaRecallParam = "Recall";
inline constexpr const char* kMetaTemperatureParam = "Temperature";
inline constexpr double kMetaTemperatureMax = 2.0;
inline bool isMetapadAction(const std::string& param) {
    return param == kMetaInterpolateParam || param == kMetaSnapshotAction;
}

inline bool isHostSwitchTarget(const std::string& param) {
    return param == kBypassParam || param == kTrackMuteParam
        || param == kSoloParam || param == kArmParam
        || param == kRandomAction || isPresetStepAction(param)
        || isTransportAction(param);
}

inline bool modelBypassed(const OrganismModel& cm) {
    for (const auto& p : cm.properties)
        if (p.name == kBypassParam) return p.value >= 0.5;
    return false;
}

inline bool modelTrackMuted(const OrganismModel& cm) {
    for (const auto& p : cm.properties)
        if (p.name == kTrackMuteParam) return p.value >= 0.5;
    return false;
}

inline std::vector<Parameter> settingsOnly(const std::vector<Parameter>& props) {
    std::vector<Parameter> out;
    out.reserve(props.size());
    for (const auto& p : props)
        if (p.name != kBypassParam && p.name != kTrackMuteParam) out.push_back(p);
    return out;
}

struct ConnectionModel {
    std::string src;
    int srcOutlet = 0;
    std::string dst;
    int dstInlet = 0;
    int midiChannel = 0;
};

struct PerformanceBox {
    std::string organism;
    double startBeat = 0.0;
    double endBeat = 0.0;
};

struct ClockModel {
    double tempo = 120.0;
    double loopStart = 0.0;
    double loopEnd = 0.0;
    bool loopEnabled = false;
    std::string timeSignature;
    double songLength = 0.0;
};

inline int timeSigNumeratorOf(const std::string& payload) {
    if (payload.empty()) return 4;
    std::string row = payload.substr(0, payload.find('\n'));
    std::string field;
    if (const auto slash = row.find('/'); slash != std::string::npos) {
        field = row.substr(0, slash);
    } else {
        const auto a = row.find('\t');
        if (a == std::string::npos) return 4;
        const auto b = row.find('\t', a + 1);
        field = row.substr(a + 1, b == std::string::npos ? std::string::npos : b - a - 1);
    }
    int total = 0;
    std::string term;
    for (size_t i = 0; i <= field.size(); ++i) {
        if (i == field.size() || field[i] == '+') {
            const int v = std::atoi(term.c_str());
            if (v > 0) total += v;
            term.clear();
        } else if (!std::isspace((unsigned char) field[i])) {
            term += field[i];
        }
    }
    return total > 0 ? total : 4;
}

inline std::string makeTimeSignature(int numerator, int denominator) {
    return "0\t" + std::to_string(numerator) + "\t|\t" + std::to_string(denominator);
}

struct OrganismView {
    std::string organismName;
    int patcherX = 0;
    int patcherY = 0;
    bool hasEditor = false;
    bool editorVisible = false;
    int editorX = 0;
    int editorY = 0;
    int editorMode = -1;
    int editorW = 0;
    int editorH = 0;
    int editorHalf = -1;
    bool editorCollapsed = false;
};

struct AutomationView {
    std::string organismName;
    std::string propertyName;
    int propertyIndex = -1;
    int index = 0;
    int height = 50;
    bool snapTo = false;
};

struct SnapshotValue {
    int propertyIndex = -1;
    std::string type = "double";
    double value = 0.0;
    double value2 = 0.0;
};
struct SnapshotOrganism {
    std::string organismName;
    std::vector<SnapshotValue> values;
};
struct SnapshotPattern {
    std::string organismName;
    Pattern pattern;
};

struct DocumentSnapshot {
    int index = 0;
    std::string name;
    std::string colour;
    std::vector<SnapshotOrganism> organisms;
    std::vector<SnapshotPattern> patterns;
};
struct MetapadPoint {
    int snapshotIndex = 0;
    double x = 0.0, y = 0.0;
};
struct MetapadMaskEntry {
    std::string organismName;
    int propertyIndex = -1;
    bool restore = true;
};
struct MorphPathPoint { double beat = 0.0, x = 0.5, y = 0.5; };

struct MetapadModel {
    bool present = false;
    std::vector<DocumentSnapshot> snapshots;
    std::vector<MetapadPoint> points;
    std::vector<MetapadMaskEntry> mask;
    int interpolateMode = 0;
    double temperature = 1.0;
    std::vector<MorphPathPoint> morphPath;
};

struct MidiModifierModel {
    int source = -1;
    bool latching = false;
    bool ownAction = false;
};

struct PatchDocumentModel {
    std::string version;
    std::vector<MidiModifierModel> midiModifiers;
    std::string applicationPath;
    std::string documentPath;
    ClockModel clock;
    std::vector<OrganismModel> organisms;
    std::vector<ConnectionModel> connections;
    std::vector<ConnectionModel> midiConnections;
    std::vector<ConnectionModel> videoConnections;
    std::vector<OrganismView> views;
    std::vector<AutomationView> automationViews;
    std::vector<PerformanceBox> perfBoxes;
    MetapadModel metapad;
    std::string notes;

    double masterLevel = 1.0;
    bool masterLimiter = false;
    double groove = 0.0;
    std::string grooveUnit = "1/16";

    const OrganismModel* byName(const std::string& n) const {
        for (auto& c : organisms)
            if (c.name == n) return &c;
        return nullptr;
    }
};

bool parsePatchFile(const std::string& path, PatchDocumentModel& out, std::string& error,
                  std::unique_ptr<juce::XmlElement>* rawOut = nullptr);
bool parsePatchText(const std::string& xmlText, PatchDocumentModel& out, std::string& error,
                  std::unique_ptr<juce::XmlElement>* rawOut = nullptr);

}
