#pragma once
#include <functional>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/Categories.h"
#include "core/ParamSchema.h"
#include "core/ParamUnit.h"
#include "gui/EngineHost.h"
#include "gui/NodeRandomize.h"
#include "gui/FollowPick.h"
#include "gui/QuickMapWindow.h"
#include "gui/RangePrompt.h"

namespace hum {

class OrganismEditor : public juce::Component {
public:
    ~OrganismEditor() override = default;

    virtual void reloadValues() = 0;
    virtual void refreshAutomatedValues() = 0;
    virtual void reloadTextValues() {}
    virtual int preferredContentWidth() const = 0;
    virtual int preferredContentHeight(int width) const = 0;
    virtual void openClip(int) {}

    std::function<void()> onAutomationChanged;

    void setWearsCollar(bool w) {
        if (wearsCollar_ == w) return;
        wearsCollar_ = w;
        resized();
        repaint();
    }
    bool wearsCollar() const { return wearsCollar_; }

private:
    bool wearsCollar_ = true;
};

class MidiLearner : private juce::Timer {
public:
    static MidiLearner& instance() { static MidiLearner l; return l; }

    std::function<void(const juce::String&)> onStatus;

    std::function<void()> onCaptured;

    void arm(EngineHost& host, std::string organism, std::string param, double min, double max) {
        onCaptured = nullptr;
        host_ = &host; organism_ = std::move(organism); param_ = std::move(param);
        min_ = min; max_ = max;
        armedAt_ = juce::Time::getMillisecondCounter();
        host.midi().clearLastCC();
        if (onStatus)
            onStatus("MIDI Learn: move a controller or play a note for " + juce::String(param_)
                     + " (waiting " + juce::String((int) (kTimeoutMs / 1000)) + "s)");
        window_ = std::make_unique<QuickMapWindow>(
            "Quick Map MIDI Control",
            "Move a controller or play a note on your MIDI device\nto control\n"
                + juce::String(organism_) + " / " + juce::String(param_),
            [] { MidiLearner::instance().cancel(); });
        window_->setCountdown((int) (kTimeoutMs / 1000));
        startTimerHz(20);
    }
    bool armed() const { return host_ != nullptr; }

    void cancel() {
        if (host_ != nullptr && onStatus) onStatus("MIDI Learn cancelled");
        disarm();
    }

private:
    static constexpr juce::uint32 kTimeoutMs = 15000;

    void disarm() {
        onCaptured = nullptr;
        host_ = nullptr;
        stopTimer();
        window_.reset();
    }

    void timerCallback() override {
        if (!host_) { disarm(); return; }
        const auto elapsed = juce::Time::getMillisecondCounter() - armedAt_;
        if (elapsed > kTimeoutMs) {
            if (onStatus) onStatus("MIDI Learn timed out (nothing received)");
            disarm();
            return;
        }
        if (window_) window_->setCountdown((int) ((kTimeoutMs - elapsed) / 1000) + 1);
        const int cc = host_->midi().lastCC();
        if (cc < 0) return;
        auto* host = host_;
        const auto org = organism_, prm = param_;
        const double lo = min_, hi = max_;
        const auto others = host->midi().map().usersOf(cc, org, prm);
        auto status = onStatus;
        auto next = std::move(onCaptured);
        disarm();
        auto commit = [host, cc, org, prm, lo, hi, status, next](bool steal) {
            const auto stolen = host->midi().mapCC(cc, org, prm, lo, hi, steal);
            if (status)
                status("MIDI: mapped " + juce::String(midiSourceLabel(cc)) + " to "
                       + juce::String(org) + " / " + juce::String(prm)
                       + (stolen.isNotEmpty() ? " (reassigned from " + stolen + ")" : ""));
            if (next) next();
        };
        if (others.empty()) { commit(true); return; }
        mapconflict::ask("MIDI", juce::String(midiSourceLabel(cc)),
                         juce::String(org) + " / " + juce::String(prm), others, commit);
    }
    EngineHost* host_ = nullptr;
    std::string organism_, param_;
    double min_ = 0.0, max_ = 1.0;
    juce::uint32 armedAt_ = 0;
    std::unique_ptr<QuickMapWindow> window_;
};

class OscLearner : private juce::Timer {
public:
    static OscLearner& instance() { static OscLearner l; return l; }

    std::function<void(const juce::String&)> onStatus;

    void arm(EngineHost& host, std::string organism, std::string param, double min, double max) {
        host_ = &host; organism_ = std::move(organism); param_ = std::move(param);
        min_ = min; max_ = max;
        armedAt_ = juce::Time::getMillisecondCounter();
        host.osc().clearLastAddress();
        if (onStatus)
            onStatus("OSC Learn: move an OSC control for " + juce::String(param_)
                     + " (port " + juce::String(host.osc().port()) + ", waiting "
                     + juce::String((int) (kTimeoutMs / 1000)) + "s)");
        window_ = std::make_unique<QuickMapWindow>(
            "Quick Map OSC Control",
            "Move an OSC control (port " + juce::String(host.osc().port())
                + ") to control\n" + juce::String(organism_) + " / " + juce::String(param_),
            [] { OscLearner::instance().cancel(); });
        window_->setCountdown((int) (kTimeoutMs / 1000));
        startTimerHz(20);
    }

    void cancel() {
        if (host_ != nullptr && onStatus) onStatus("OSC Learn cancelled");
        disarm();
    }

private:
    static constexpr juce::uint32 kTimeoutMs = 15000;

    void disarm() {
        host_ = nullptr;
        stopTimer();
        window_.reset();
    }

    void timerCallback() override {
        if (!host_) { disarm(); return; }
        const auto elapsed = juce::Time::getMillisecondCounter() - armedAt_;
        if (elapsed > kTimeoutMs) {
            if (onStatus) onStatus("OSC Learn timed out (nothing received)");
            disarm();
            return;
        }
        if (window_) window_->setCountdown((int) ((kTimeoutMs - elapsed) / 1000) + 1);
        const auto addr = host_->osc().lastAddress();
        if (addr.isEmpty()) return;
        auto* host = host_;
        const auto org = organism_, prm = param_;
        const double lo = min_, hi = max_;
        const auto address = addr.toStdString();
        const auto others = host->osc().map().usersOf(address, org, prm);
        auto status = onStatus;
        disarm();
        auto commit = [host, address, addr, org, prm, lo, hi, status](bool steal) {
            const auto stolen = host->osc().mapAddress(address, org, prm, lo, hi, steal);
            if (status)
                status("OSC: mapped " + addr + " to " + juce::String(org) + " / " + juce::String(prm)
                       + (stolen.isNotEmpty() ? " (reassigned from " + stolen + ")" : ""));
        };
        if (others.empty()) { commit(true); return; }
        mapconflict::ask("OSC", addr, juce::String(org) + " / " + juce::String(prm), others, commit);
    }
    EngineHost* host_ = nullptr;
    std::string organism_, param_;
    double min_ = 0.0, max_ = 1.0;
    juce::uint32 armedAt_ = 0;
    std::unique_ptr<QuickMapWindow> window_;
};

inline std::pair<double, double> paramRange(EngineHost& host, const std::string& organism,
                                            const std::string& param) {
    if (auto* cm = host.model().byName(organism)) {
        if (isClockPseudo(cm->displayClass) && param == kTempoParam)
            return {kTempoMin, kTempoMax};
        if (isMetapadPseudo(cm->displayClass) && param == kMetaTemperatureParam)
            return {0.0, kMetaTemperatureMax};
        for (const auto& d : schemaFor(cm->classRaw))
            if (d.name == param) return {d.min, d.max};
    }
    return {0.0, 1.0};
}

inline Unit paramUnit(EngineHost& host, const std::string& organism,
                      const std::string& param) {
    if (auto* cm = host.model().byName(organism)) {
        if (isClockPseudo(cm->displayClass) && param == kTempoParam) return Unit::Bpm;
        for (const auto& d : schemaFor(cm->classRaw))
            if (d.name == param) return unitResolve(d.name, d.unit, d.min, d.max);
    }
    return Unit::None;
}

inline std::vector<std::string> controlTargets(EngineHost& host, const std::string& organism) {
    std::vector<std::string> out;
    const auto* cm = host.model().byName(organism);
    if (cm == nullptr) return out;
    if (isClockPseudo(cm->displayClass)) {
        out.push_back(kTempoParam);
        out.push_back(kRandomAction);
        for (const char* a : {kPlayAction, kStopAction, kPlayFromStartAction,
                              kGoToStartAction, kGoToEndAction, kCaptureAction,
                              kLoopToggleAction})
            out.push_back(a);
        return out;
    }
    if (isMetapadPseudo(cm->displayClass)) {
        out.push_back(kMetaXParam);
        out.push_back(kMetaYParam);
        if (!host.model().metapad.snapshots.empty()) out.push_back(kMetaRecallParam);
        out.push_back(kMetaTemperatureParam);
        out.push_back(kMetaInterpolateParam);
        out.push_back(kMetaSnapshotAction);
        return out;
    }
    for (const auto& d : schemaFor(cm->classRaw))
        if (!d.isText) out.push_back(d.name);
    out.push_back(kBypassParam);
    if (nodeSupportsRandom(host, organism)) out.push_back(kRandomAction);
    if (!cm->presets.empty()) {
        out.push_back(kPresetNextAction);
        out.push_back(kPresetPrevAction);
    }
    return out;
}

inline juce::String targetOwnerLabel(EngineHost& host, const std::string& organism) {
    const auto* cm = host.model().byName(organism);
    const auto pseudo = cm != nullptr ? pseudoOwnerLabel(cm->displayClass) : std::string();
    return pseudo.empty() ? juce::String(organism) : juce::String(pseudo);
}

class FollowPicker {
public:
    static FollowPicker& instance() { static FollowPicker p; return p; }

    std::function<void(const juce::String&)> onStatus;

    void arm(EngineHost& host, std::string organism, std::string param,
             double lo, double hi, std::function<void()> onChanged) {
        const auto target = juce::String(organism) + " / " + juce::String(param);
        if (onStatus) onStatus("Follow: click the control that should move " + target);
        window_ = std::make_unique<QuickMapWindow>(
            "Follow a Control",
            "Click any knob to make it move\n" + target,
            [] { juce::MessageManager::callAsync([] { FollowPicker::instance().cancel(); }); });
        followpick::arm([this, &host, organism, param, lo, hi, onChanged]
                        (const std::string& from, const std::string& value) {
            close();
            if (from == organism && value == param) {
                if (onStatus) onStatus("Follow: a control cannot follow itself");
                return;
            }
            const auto src = std::string(kParamSource) + value;
            const auto title = juce::String(from) + " / " + juce::String(value)
                             + juce::String::fromUTF8(" \xe2\x86\x92 ") + juce::String(param);
            auto status = onStatus;
            const auto at = juce::Desktop::getMousePosition();
            juce::MessageManager::callAsync([&host, organism, param, from, src, title,
                                             lo, hi, at, status, onChanged] {
                RangePrompt::show({at.x, at.y, 1, 1}, title, paramUnit(host, organism, param),
                                  lo, hi,
                                  [&host, organism, param, from, src, title, status,
                                   onChanged](double a, double b) {
                                      host.mod().mapRoute(from, src, organism, param, a, b);
                                      if (status) status("Follow: " + title);
                                      if (onChanged) onChanged();
                                  });
            });
        });
    }

    void cancel() {
        if (followpick::armed() && onStatus) onStatus("Follow cancelled");
        close();
    }

private:
    void close() {
        followpick::cancel();
        window_.reset();
    }
    std::unique_ptr<QuickMapWindow> window_;
};

inline void showAutomateMenu(EngineHost& host, const std::string& organism,
                             const std::string& param, juce::Point<int> screen,
                             std::function<void()> onChanged,
                             bool allowAutomate = true,
                             std::function<void(bool)> automateOverride = {}) {
    const bool automated = allowAutomate && host.automation().isAutomated(organism, param);
    const auto range = paramRange(host, organism, param);
    const double lo = range.first, hi = range.second;
    bool mapped = false;
    for (const auto& e : host.midi().map().entries())
        if (e.organism == organism && e.param == param) { mapped = true; break; }
    bool oscMapped = false;
    for (const auto& e : host.osc().map().entries())
        if (e.organism == organism && e.param == param) { oscMapped = true; break; }
    bool modMapped = false, followMapped = false;
    for (const auto& e : host.mod().map().entries()) {
        if (e.organism != organism || e.param != param) continue;
        (isParamSource(e.value) ? followMapped : modMapped) = true;
    }

    const auto sources = host.mod().availableSources();

    juce::PopupMenu m;
    const bool rollable = paramSupportsRandom(host, organism, param);
    const bool lockable = rollTouchesParam(host, organism, param);
    const bool locked = lockable && host.rollLocked(organism, param);
    if (rollable || lockable) {
        if (rollable) m.addItem(10, "Random");
        if (lockable) m.addItem(11, "Exclude from Random", true, locked);
        m.addSeparator();
    }
    if (allowAutomate) {
        m.addItem(1, automated ? "Unautomate" : "Automate");
        m.addSeparator();
    }
    m.addItem(3, "MIDI Learn");
    m.addItem(4, "Clear MIDI", mapped);
    if (host.osc().enabled() || oscMapped) {
        m.addSeparator();
        m.addItem(5, "OSC Learn", host.osc().enabled());
        m.addItem(6, "Clear OSC", oscMapped);
    }
    {
        juce::PopupMenu mod, follow, modNode, followNode;
        std::string modOpen, followOpen;
        auto flush = [](juce::PopupMenu& into, juce::PopupMenu& node, std::string& open) {
            if (!open.empty()) into.addSubMenu(juce::String(open.c_str()), node);
            node.clear();
            open.clear();
        };
        for (size_t i = 0; i < sources.size(); ++i) {
            const auto& [from, value] = sources[i];
            const bool isParam = isParamSource(value);
            const auto shown = paramSourceName(value);
            if (from == organism && shown == param) continue;
            auto& into = isParam ? follow : mod;
            auto& node = isParam ? followNode : modNode;
            auto& open = isParam ? followOpen : modOpen;
            if (from != open) { flush(into, node, open); open = from; }
            bool routed = false;
            for (const auto& e : host.mod().map().entries())
                if (e.organism == organism && e.param == param
                    && e.source == from && e.value == value)
                    routed = true;
            node.addItem(100 + (int) i, juce::String(shown.c_str()), true, routed);
        }
        flush(mod, modNode, modOpen);
        flush(follow, followNode, followOpen);
        juce::PopupMenu followTop;
        followTop.addItem(13, juce::String::fromUTF8("Pick a control\xe2\x80\xa6"));
        if (follow.getNumItems() > 0) {
            followTop.addSeparator();
            followTop.addSubMenu("From a list", follow);
        }
        m.addSeparator();
        m.addSubMenu("Control with", mod, mod.getNumItems() > 0);
        if (modMapped) m.addItem(12, "Release control");
        m.addSeparator();
        m.addSubMenu("Follow", followTop);
        if (followMapped) m.addItem(8, "Stop following");
    }
    m.addSeparator();
    m.addItem(9, "Clear all control",
              automated || mapped || oscMapped || modMapped || followMapped);
    if (host.openParameterControl) {
        m.addSeparator();
        m.addItem(7, "Parameter Control...");
    }
    m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({screen.x, screen.y, 1, 1}),
                    [&host, organism, param, automated, locked, lo, hi, onChanged, sources,
                     automateOverride, screen](int r) {
        if (r == 1) {
            if (automateOverride)   automateOverride(!automated);
            else if (automated)     host.automation().remove(organism, param);
            else                    host.automation().add(organism, param);
            if (onChanged) onChanged();
        } else if (r == 10) {
            randomizeParam(host, organism, param);
            if (onChanged) onChanged();
        } else if (r == 11) {
            host.setRollLocked(organism, param, !locked);
            if (onChanged) onChanged();
        } else if (r == 3) {
            MidiLearner::instance().arm(host, organism, param, lo, hi);
        } else if (r == 4) {
            std::vector<int> ccs;
            for (const auto& e : host.midi().map().entries())
                if (e.organism == organism && e.param == param) ccs.push_back(e.cc);
            for (int cc : ccs) host.midi().clearCC(cc, organism, param);
        } else if (r == 5) {
            OscLearner::instance().arm(host, organism, param, lo, hi);
        } else if (r == 6) {
            std::vector<std::string> addrs;
            for (const auto& e : host.osc().map().entries())
                if (e.organism == organism && e.param == param) addrs.push_back(e.address);
            for (const auto& a : addrs) host.osc().clearAddress(a, organism, param);
        } else if (r == 13) {
            juce::MessageManager::callAsync([&host, organism, param, lo, hi, onChanged] {
                FollowPicker::instance().arm(host, organism, param, lo, hi, onChanged);
            });
        } else if (r == 7) {
            if (host.openParameterControl) host.openParameterControl(organism, param);
        } else if (r == 8 || r == 12 || r == 9) {
            std::vector<std::pair<std::string, std::string>> routes;
            for (const auto& e : host.mod().map().entries()) {
                if (e.organism != organism || e.param != param) continue;
                if (r != 9 && isParamSource(e.value) != (r == 8)) continue;
                routes.push_back({e.source, e.value});
            }
            for (const auto& sv : routes)
                host.mod().clearRoute(sv.first, sv.second, organism, param);
            if (r == 9) {
                if (automated) host.automation().remove(organism, param);
                std::vector<int> ccs;
                for (const auto& e : host.midi().map().entries())
                    if (e.organism == organism && e.param == param) ccs.push_back(e.cc);
                for (int cc : ccs) host.midi().clearCC(cc, organism, param);
                std::vector<std::string> addrs;
                for (const auto& e : host.osc().map().entries())
                    if (e.organism == organism && e.param == param) addrs.push_back(e.address);
                for (const auto& a : addrs) host.osc().clearAddress(a, organism, param);
            }
            if (onChanged) onChanged();
        } else if (r >= 100 && r < 100 + (int) sources.size()) {
            const auto& sv = sources[(size_t) (r - 100)];
            bool routed = false;
            for (const auto& e : host.mod().map().entries())
                if (e.organism == organism && e.param == param
                    && e.source == sv.first && e.value == sv.second)
                    routed = true;
            if (routed) {
                host.mod().clearRoute(sv.first, sv.second, organism, param);
                if (onChanged) onChanged();
            } else {
                const juce::Rectangle<int> at(screen.x, screen.y, 1, 1);
                const auto u = paramUnit(host, organism, param);
                const auto title = juce::String(sv.first) + " -> " + param;
                juce::MessageManager::callAsync([&host, at, title, u, lo, hi, organism,
                                                 param, sv, onChanged] {
                    RangePrompt::show(at, title, u, lo, hi,
                                      [&host, organism, param, sv, onChanged](double a, double b) {
                                          host.mod().mapRoute(sv.first, sv.second, organism,
                                                              param, a, b);
                                          if (onChanged) onChanged();
                                      });
                });
            }
        }
    });
}

}
