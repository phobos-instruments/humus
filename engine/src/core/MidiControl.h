#pragma once
#include <string>
#include <utility>
#include <vector>

#include "core/ControlShape.h"

namespace hum {

inline constexpr int kMidiSourceCount = 256;
inline constexpr int kNoteSourceBase = 128;
inline bool isNoteSource(int id) { return id >= kNoteSourceBase && id < kMidiSourceCount; }
inline int noteOfSource(int id) { return id - kNoteSourceBase; }
inline int sourceForNote(int note) { return kNoteSourceBase + note; }
inline std::string midiSourceLabel(int id) {
    return isNoteSource(id) ? "Note " + std::to_string(noteOfSource(id))
                            : "CC " + std::to_string(id);
}

struct MidiMapEntry {
    int cc = 0;
    std::string organism;
    std::string param;
    double min = 0.0;
    double max = 1.0;
    ControlShape shape;
    ControlShapeState state;
};

struct MidiParamUpdate {
    std::string organism;
    std::string param;
    double value = 0.0;
};

class MidiControlMap {
public:
    void set(int cc, const std::string& organism, const std::string& param,
             double min, double max) {
        for (auto& e : entries_)
            if (e.cc == cc && e.organism == organism && e.param == param) {
                e.min = min; e.max = max; return;
            }
        MidiMapEntry e;
        e.cc = cc; e.organism = organism; e.param = param; e.min = min; e.max = max;
        entries_.push_back(std::move(e));
    }

    void setShape(int cc, const std::string& organism, const std::string& param,
                  const ControlShape& shape) {
        for (auto& e : entries_)
            if (e.cc == cc && e.organism == organism && e.param == param) {
                e.shape = shape;
                e.state = ControlShapeState{};
                return;
            }
    }
    const ControlShape* shapeOf(int cc, const std::string& organism,
                                const std::string& param) const {
        for (auto& e : entries_)
            if (e.cc == cc && e.organism == organism && e.param == param) return &e.shape;
        return nullptr;
    }

    std::vector<std::pair<std::string, std::string>>
    steal(int cc, const std::string& keepOrganism, const std::string& keepParam) {
        std::vector<std::pair<std::string, std::string>> stolen;
        for (const auto& e : entries_)
            if (e.cc == cc && !(e.organism == keepOrganism && e.param == keepParam))
                stolen.push_back({e.organism, e.param});
        erase([&](const MidiMapEntry& e) {
            return e.cc == cc && !(e.organism == keepOrganism && e.param == keepParam);
        });
        return stolen;
    }

    std::vector<std::pair<std::string, std::string>>
    usersOf(int cc, const std::string& exceptOrganism, const std::string& exceptParam) const {
        std::vector<std::pair<std::string, std::string>> out;
        for (const auto& e : entries_)
            if (e.cc == cc && !(e.organism == exceptOrganism && e.param == exceptParam))
                out.push_back({e.organism, e.param});
        return out;
    }

    void clear(int cc, const std::string& organism, const std::string& param) {
        erase([&](const MidiMapEntry& e) {
            return e.cc == cc && e.organism == organism && e.param == param;
        });
    }
    void clearOrganism(const std::string& organism) {
        erase([&](const MidiMapEntry& e) { return e.organism == organism; });
    }
    void renameOrganism(const std::string& oldName, const std::string& newName) {
        for (auto& e : entries_) if (e.organism == oldName) e.organism = newName;
    }
    void clearAll() { entries_.clear(); }

    bool empty() const { return entries_.empty(); }
    const std::vector<MidiMapEntry>& entries() const { return entries_; }

    std::vector<MidiParamUpdate> resolve(int cc, int value7) const {
        std::vector<MidiParamUpdate> out;
        const double t = (double) clamp7(value7) / 127.0;
        for (const auto& e : entries_)
            if (e.cc == cc)
                out.push_back({e.organism, e.param, e.min + (e.max - e.min) * t});
        return out;
    }

    std::vector<MidiParamUpdate> tick(const int* sourceValues, const bool* fresh, double dtSeconds) {
        std::vector<MidiParamUpdate> out;
        for (auto& e : entries_) {
            const int raw = e.cc >= 0 && e.cc < kMidiSourceCount ? sourceValues[e.cc] : -1;
            if (raw < 0) continue;
            if (e.state.lastOut < 0.0 && !fresh[e.cc]) continue;
            const double shaped = advanceControlShape(e.shape, e.state, raw / 127.0, dtSeconds);
            if (shaped < 0.0) continue;
            out.push_back({e.organism, e.param, shapedToRange(e.shape, e.min, e.max, shaped)});
        }
        return out;
    }

    static int clamp7(int v) { return v < 0 ? 0 : (v > 127 ? 127 : v); }

private:
    template <typename Pred>
    void erase(Pred p) {
        std::vector<MidiMapEntry> kept;
        for (auto& e : entries_) if (!p(e)) kept.push_back(e);
        entries_.swap(kept);
    }
    std::vector<MidiMapEntry> entries_;
};

struct MidiOutMessage {
    int status = 0;
    int data1 = 0;
    int data2 = 0;
};

inline std::vector<MidiOutMessage> midiOutMessages(const std::string& changedParam,
                                                   int channel, int controller, int value,
                                                   int note, int velocity, double gate) {
    const int ch = (MidiControlMap::clamp7(channel < 1 ? 1 : channel) - 1) & 0x0F;
    std::vector<MidiOutMessage> out;
    if (changedParam == "Value" || changedParam == "Controller")
        out.push_back({0xB0 | ch, MidiControlMap::clamp7(controller), MidiControlMap::clamp7(value)});
    else if (changedParam == "Gate")
        out.push_back(gate >= 0.5
                          ? MidiOutMessage{0x90 | ch, MidiControlMap::clamp7(note), MidiControlMap::clamp7(velocity)}
                          : MidiOutMessage{0x80 | ch, MidiControlMap::clamp7(note), 0});
    return out;
}

}
