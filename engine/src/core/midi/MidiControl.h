// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "core/midi/MidiSource.h"
#include "core/midi/MidiSourceFrame.h"
#include "core/net/ControlShape.h"

#include "hum/dsp/DspMath.h"

namespace hum {

struct MidiMapEntry {
    int cc = 0;
    std::vector<int> held;
    int port = kAnyMidiPort;
    int channel = kAnyMidiChannel;
    std::string organism;
    std::string param;
    double min = 0.0;
    double max = 1.0;
    ControlShape shape;
    ControlShapeState state;
    bool engaged = false;
    bool wasPressed = false;

    MidiSource source() const { return MidiSource(cc, held, port, channel); }
    int lane() const { return midiLane(port, channel); }
    bool is(const MidiSource& s, const std::string& org, const std::string& prm) const {
        return cc == s.cc && held == s.held && lane() == s.lane()
               && organism == org && param == prm;
    }
};

struct MidiModifier {
    int source = -1;
    bool latching = false;
    bool ownAction = false;
    bool on = false;
    bool wasPressed = false;
};

using MidiParamUpdate = ControlUpdate;
using CurrentValueFn = std::function<double(const std::string&, const std::string&)>;

class MidiControlMap {
public:
    void set(const MidiSource& src, const std::string& organism, const std::string& param,
             double min, double max) {
        for (auto& e : entries_)
            if (e.is(src, organism, param)) { e.min = min; e.max = max; return; }
        MidiMapEntry e;
        e.cc = src.cc; e.held = src.held; e.port = src.port; e.channel = src.channel;
        e.organism = organism; e.param = param;
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

    std::vector<MidiParamUpdate> tick(const MidiSourceFrame& frame, double dtSeconds,
                                      const CurrentValueFn& currentOf = nullptr) {
        prepareLanes(frame);
        std::vector<MidiParamUpdate> out;
        for (auto& e : entries_) {
            if (!isMidiSource(e.cc)) continue;
            const int lane = e.lane();
            const bool active = isActive(e);
            if (e.shape.isEncoder()) {
                if (!active) continue;
                const auto a = encoderAction(e.shape, frame.encoderTicks(e.shape.encoder, lane, e.cc));
                if (!a.none()) out.push_back(controlUpdate(e.organism, e.param, e.shape, e.min, e.max, a));
                continue;
            }
            const int raw = frame.value(lane, e.cc);
            const bool ready = frame.fresh(lane, e.cc);
            if (raw < 0) continue;
            if (e.state.lastOut < 0.0 && !e.state.seen && !ready) continue;
            const double t = raw / (double) sourceMaxValue(e.cc);
            if (e.shape.isButton()) {
                if (!consumesPress(e, active, t)) continue;
            } else if (!active) {
                if (ready || e.state.lastOut < 0.0) trackSilently(e, t);
                continue;
            }
            const auto a = advanceControl(e.shape, e.state, t, dtSeconds);
            if (a.none()) continue;
            if (!pickedUp(e, a, currentOf)) continue;
            out.push_back(controlUpdate(e.organism, e.param, e.shape, e.min, e.max, a));
        }
        return out;
    }

    static int clamp7(int v) { return v < 0 ? 0 : (v > kMidiMax ? kMidiMax : v); }

private:
    struct LaneState {
        int lane = 0;
        std::vector<char> held;
        std::vector<int> best;
    };

    void prepareLanes(const MidiSourceFrame& frame) {
        lanes_.clear();
        for (const auto& e : entries_) {
            const int lane = e.lane();
            bool known = false;
            for (const auto& l : lanes_) known = known || l.lane == lane;
            if (!known) lanes_.push_back({lane, std::vector<char>(kMidiSourceCount, 0),
                                          std::vector<int>(kMidiSourceCount, -1)});
        }
        const int* any = frame.laneValues(midiLane(kAnyMidiPort, kAnyMidiChannel));
        for (auto& m : modifiers_) {
            if (!canBeHeld(m.source)) continue;
            const bool pressed = any[m.source] > kHeldThreshold;
            if (m.latching && pressed && !m.wasPressed) m.on = !m.on;
            m.wasPressed = pressed;
        }
        for (auto& l : lanes_) {
            const int* v = frame.laneValues(l.lane);
            for (int s = 0; s < kMidiSourceCount; ++s)
                l.held[(std::size_t) s] = canBeHeld(s) && v[s] > kHeldThreshold ? 1 : 0;
            for (const auto& m : modifiers_)
                if (m.latching && canBeHeld(m.source)) l.held[(std::size_t) m.source] = m.on ? 1 : 0;
        }
        for (const auto& e : entries_) {
            if (!isMidiSource(e.cc)) continue;
            auto& l = laneOf(e.lane());
            if (eligible(e, l.held)) {
                auto& b = l.best[(std::size_t) e.cc];
                b = std::max(b, (int) e.held.size());
            }
        }
    }

    LaneState& laneOf(int lane) {
        for (auto& l : lanes_) if (l.lane == lane) return l;
        return lanes_.front();
    }

    bool isActive(const MidiMapEntry& e) {
        auto& l = laneOf(e.lane());
        return eligible(e, l.held) && (int) e.held.size() == l.best[(std::size_t) e.cc];
    }

    bool eligible(const MidiMapEntry& e, const std::vector<char>& held) const {
        for (int h : e.held)
            if (!canBeHeld(h) || held[(std::size_t) h] == 0) return false;
        if (e.held.empty())
            if (const auto* m = modifierOf(e.cc); m != nullptr && !m->ownAction) return false;
        return true;
    }

    static bool pickedUp(MidiMapEntry& e, const ControlAction& a, const CurrentValueFn& currentOf) {
        if (!e.shape.picksUp() || !currentOf || a.kind != ControlActionKind::Absolute) return true;
        const double current = currentOf(e.organism, e.param);
        if (std::isnan(current)) return true;
        return pickUpAllows(e.state, a.value, rangeToShaped(e.shape, e.min, e.max, current));
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
    std::vector<LaneState> lanes_;
};

}
