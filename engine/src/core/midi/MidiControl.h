// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <utility>
#include <vector>

#include "core/net/ControlShape.h"
#include "core/midi/MidiSource.h"

#include "hum/dsp/DspMath.h"

namespace hum {

struct MidiMapEntry {
    int cc = 0;
    std::vector<int> held;
    std::string organism;
    std::string param;
    double min = 0.0;
    double max = 1.0;
    ControlShape shape;
    ControlShapeState state;
    bool engaged = false;
    bool wasPressed = false;

    MidiSource source() const { return MidiSource(cc, held); }
    bool is(const MidiSource& s, const std::string& org, const std::string& prm) const {
        return cc == s.cc && held == s.held && organism == org && param == prm;
    }
};

struct MidiModifier {
    int source = -1;
    bool latching = false;
    bool ownAction = false;
    bool on = false;
    bool wasPressed = false;
};

struct MidiParamUpdate {
    std::string organism;
    std::string param;
    double value = 0.0;
};

class MidiControlMap {
public:
    void set(const MidiSource& src, const std::string& organism, const std::string& param,
             double min, double max) {
        for (auto& e : entries_)
            if (e.is(src, organism, param)) { e.min = min; e.max = max; return; }
        MidiMapEntry e;
        e.cc = src.cc; e.held = src.held; e.organism = organism; e.param = param;
        e.min = min; e.max = max;
        entries_.push_back(std::move(e));
        syncModifiers();
    }
    void set(int cc, const std::string& organism, const std::string& param,
             double min, double max) {
        set(MidiSource(cc), organism, param, min, max);
    }

    void setShape(const MidiSource& src, const std::string& organism, const std::string& param,
                  const ControlShape& shape) {
        for (auto& e : entries_)
            if (e.is(src, organism, param)) {
                e.shape = shape;
                e.state = ControlShapeState{};
                e.engaged = false;
                return;
            }
    }
    void setShape(int cc, const std::string& organism, const std::string& param,
                  const ControlShape& shape) {
        setShape(MidiSource(cc), organism, param, shape);
    }
    const ControlShape* shapeOf(const MidiSource& src, const std::string& organism,
                                const std::string& param) const {
        for (auto& e : entries_)
            if (e.is(src, organism, param)) return &e.shape;
        return nullptr;
    }
    const ControlShape* shapeOf(int cc, const std::string& organism,
                                const std::string& param) const {
        return shapeOf(MidiSource(cc), organism, param);
    }

    std::vector<std::pair<std::string, std::string>>
    steal(const MidiSource& src, const std::string& keepOrganism, const std::string& keepParam) {
        const auto stolen = usersOf(src, keepOrganism, keepParam);
        erase([&](const MidiMapEntry& e) {
            return e.source() == src && !(e.organism == keepOrganism && e.param == keepParam);
        });
        return stolen;
    }
    std::vector<std::pair<std::string, std::string>>
    steal(int cc, const std::string& keepOrganism, const std::string& keepParam) {
        return steal(MidiSource(cc), keepOrganism, keepParam);
    }

    std::vector<std::pair<std::string, std::string>>
    usersOf(const MidiSource& src, const std::string& exceptOrganism,
            const std::string& exceptParam) const {
        std::vector<std::pair<std::string, std::string>> out;
        for (const auto& e : entries_)
            if (e.source() == src && !(e.organism == exceptOrganism && e.param == exceptParam))
                out.push_back({e.organism, e.param});
        return out;
    }
    std::vector<std::pair<std::string, std::string>>
    usersOf(int cc, const std::string& exceptOrganism, const std::string& exceptParam) const {
        return usersOf(MidiSource(cc), exceptOrganism, exceptParam);
    }

    void clear(const MidiSource& src, const std::string& organism, const std::string& param) {
        erase([&](const MidiMapEntry& e) { return e.is(src, organism, param); });
    }
    void clear(int cc, const std::string& organism, const std::string& param) {
        clear(MidiSource(cc), organism, param);
    }
    void clearOrganism(const std::string& organism) {
        erase([&](const MidiMapEntry& e) { return e.organism == organism; });
    }
    void renameOrganism(const std::string& oldName, const std::string& newName) {
        for (auto& e : entries_) if (e.organism == oldName) e.organism = newName;
    }
    void clearAll() { entries_.clear(); modifiers_.clear(); }

    bool empty() const { return entries_.empty(); }
    const std::vector<MidiMapEntry>& entries() const { return entries_; }

    const std::vector<MidiModifier>& modifiers() const { return modifiers_; }
    const MidiModifier* modifierOf(int source) const {
        for (const auto& m : modifiers_) if (m.source == source) return &m;
        return nullptr;
    }
    bool isModifier(int source) const { return modifierOf(source) != nullptr; }
    void setModifier(int source, bool latching, bool ownAction) {
        for (auto& m : modifiers_)
            if (m.source == source) {
                if (m.latching != latching) m.on = false;
                m.latching = latching;
                m.ownAction = ownAction;
                return;
            }
    }
    bool bankOn(int source) const {
        const auto* m = modifierOf(source);
        return m != nullptr && m->latching && m->on;
    }

    std::vector<MidiParamUpdate> resolve(int cc, int value7) const {
        std::vector<MidiParamUpdate> out;
        const double t = (double) clamp7(value7) / kMidiMaxD;
        for (const auto& e : entries_)
            if (e.cc == cc)
                out.push_back({e.organism, e.param, e.min + (e.max - e.min) * t});
        return out;
    }

    std::vector<MidiParamUpdate> tick(const int* sourceValues, const bool* fresh, double dtSeconds) {
        bool held[kMidiSourceCount];
        updateHeld(sourceValues, held);
        int best[kMidiSourceCount];
        for (auto& b : best) b = -1;
        for (const auto& e : entries_)
            if (isMidiSource(e.cc) && eligible(e, held))
                best[e.cc] = std::max(best[e.cc], (int) e.held.size());

        std::vector<MidiParamUpdate> out;
        for (auto& e : entries_) {
            const int raw = isMidiSource(e.cc) ? sourceValues[e.cc] : -1;
            if (raw < 0) continue;
            if (e.state.lastOut < 0.0 && !fresh[e.cc]) continue;
            const bool active = eligible(e, held) && (int) e.held.size() == best[e.cc];
            const double t = raw / kMidiMaxD;
            if (e.shape.isSwitch) {
                if (!consumesPress(e, active, t)) continue;
            } else if (!active) {
                if (fresh[e.cc] || e.state.lastOut < 0.0) trackSilently(e, t);
                continue;
            }
            const double shaped = advanceControlShape(e.shape, e.state, t, dtSeconds);
            if (shaped < 0.0) continue;
            out.push_back({e.organism, e.param, shapedToRange(e.shape, e.min, e.max, shaped)});
        }
        return out;
    }

    static int clamp7(int v) { return v < 0 ? 0 : (v > kMidiMax ? kMidiMax : v); }

private:
    void updateHeld(const int* v, bool* held) {
        for (int s = 0; s < kMidiSourceCount; ++s) held[s] = v[s] > kHeldThreshold;
        for (auto& m : modifiers_) {
            if (!isMidiSource(m.source)) continue;
            const bool pressed = v[m.source] > kHeldThreshold;
            if (m.latching) {
                if (pressed && !m.wasPressed) m.on = !m.on;
                held[m.source] = m.on;
            }
            m.wasPressed = pressed;
        }
    }

    bool eligible(const MidiMapEntry& e, const bool* held) const {
        for (int h : e.held)
            if (!isMidiSource(h) || !held[h]) return false;
        if (e.held.empty())
            if (const auto* m = modifierOf(e.cc); m != nullptr && !m->ownAction) return false;
        return true;
    }

    static bool consumesPress(MidiMapEntry& e, bool active, double t) {
        const bool pressed = (t > e.shape.threshold) != e.shape.inverted;
        const bool wasPressed = e.wasPressed;
        e.wasPressed = pressed;
        if (e.engaged) {
            if (!pressed) e.engaged = false;
            return true;
        }
        if (active && pressed && !wasPressed) { e.engaged = true; return true; }
        return false;
    }

    static void trackSilently(MidiMapEntry& e, double t) {
        e.state.smoothed = t;
        e.state.lastOut = e.shape.curveAt(t);
    }

    void syncModifiers() {
        std::vector<MidiModifier> next;
        for (const auto& e : entries_)
            for (int h : e.held) {
                bool known = false;
                for (const auto& m : next) known = known || m.source == h;
                if (known) continue;
                if (const auto* old = modifierOf(h)) next.push_back(*old);
                else next.push_back(MidiModifier{h});
            }
        modifiers_.swap(next);
    }

    template <typename Pred>
    void erase(Pred p) {
        std::vector<MidiMapEntry> kept;
        for (auto& e : entries_) if (!p(e)) kept.push_back(e);
        entries_.swap(kept);
        syncModifiers();
    }
    std::vector<MidiMapEntry> entries_;
    std::vector<MidiModifier> modifiers_;
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
