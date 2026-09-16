// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/packs/Categories.h"
#include "core/midi/MidiControl.h"
#include "core/params/ParamSchema.h"
#include "core/params/ParamUnit.h"
#include "gui/host/BrickHost.h"
#include "gui/editor/ControlDefaults.h"
#include "gui/host/EngineHostAutomation.h"
#include "gui/host/EngineHostMidiControl.h"
#include "gui/editor/FollowPick.h"
#include "gui/common/Localisation.h"
#include "gui/host/ModHost.h"
#include "gui/host/NodeRandomize.h"
#include "gui/host/OscHost.h"
#include "gui/editor/ParamLearners.h"
#include "gui/editor/QuickMapWindow.h"
#include "gui/editor/RangePrompt.h"
#include "hum/Swing.h"
#include "io/PatchDocument.h"

namespace hum {

inline std::pair<double, double> paramRange(BrickHost& host, const std::string& organism,
                                            const std::string& param) {
    if (auto* cm = host.model().byName(organism)) {
        if (isClockPseudo(cm->displayClass) && param == kTempoParam)
            return {kTempoMin, kTempoMax};
        if (isClockPseudo(cm->displayClass) && isMeterParam(param))
            return {kMeterLaneMin, kMeterLaneMax};
        if (isClockPseudo(cm->displayClass) && param == kGrooveParam) return {0.0, 1.0};
        if (isClockPseudo(cm->displayClass) && param == kGrooveGridParam)
            return {0.0, (double) (swing::kGridChoices - 1)};
        if (isMetapadPseudo(cm->displayClass) && param == kMetaTemperatureParam)
            return {0.0, kMetaTemperatureMax};
        for (const auto& d : schemaFor(cm->classRaw))
            if (d.name == param) return {d.min, d.max};
    }
    return {0.0, 1.0};
}

inline Unit paramUnit(BrickHost& host, const std::string& organism,
                      const std::string& param) {
    if (auto* cm = host.model().byName(organism)) {
        if (isClockPseudo(cm->displayClass) && param == kTempoParam) return Unit::Bpm;
        if (isClockPseudo(cm->displayClass) && param == kGrooveParam) return Unit::Percent;
        for (const auto& d : schemaFor(cm->classRaw))
            if (d.name == param) return unitResolve(d.name, d.unit, d.min, d.max);
    }
    return Unit::None;
}

inline std::vector<std::string> controlTargets(BrickHost& host, const std::string& organism) {
    std::vector<std::string> out;
    const auto* cm = host.model().byName(organism);
    if (cm == nullptr) return out;
    if (isClockPseudo(cm->displayClass)) {
        out.push_back(kTempoParam);
        out.push_back(kMeterBeatsParam);
        out.push_back(kMeterUnitParam);
        out.push_back(kGrooveParam);
        out.push_back(kGrooveGridParam);
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

inline juce::String targetOwnerLabel(BrickHost& host, const std::string& organism) {
    const auto* cm = host.model().byName(organism);
    const auto pseudo = cm != nullptr ? pseudoOwnerLabel(cm->displayClass) : std::string();
    return pseudo.empty() ? juce::String(organism) : juce::String(pseudo);
}

class FollowPicker {
public:
    static FollowPicker& instance() { static FollowPicker p; return p; }

    std::function<void(const juce::String&)> onStatus;

    void arm(BrickHost& host, std::string organism, std::string param,
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
            if (paramIsSwitch(host, organism, param)) {
                host.mod().mapRoute(from, src, organism, param, lo, hi);
                if (status) status("Follow: " + title);
                if (onChanged) onChanged();
                return;
            }
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
        if (followpick::armed() && onStatus) onStatus(tr("organism-editor.follow-cancelled", "Follow cancelled"));
        close();
    }

private:
    void close() {
        followpick::cancel();
        window_.reset();
    }
    std::unique_ptr<QuickMapWindow> window_;
};

inline void showAutomateMenu(BrickHost& host, const std::string& organism,
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
    if (paramHasDefault(host, organism, param)) {
        m.addItem(14, tr("organism-editor.reset", "Reset"));
        if (!rollable && !lockable) m.addSeparator();
    }
    if (rollable || lockable) {
        if (rollable) m.addItem(10, tr("organism-editor.random", "Random"));
        if (lockable) m.addItem(11, tr("organism-editor.exclude-from-random", "Exclude from Random"), true, locked);
        m.addSeparator();
    }
    if (allowAutomate) {
        m.addItem(1, automated ? tr("organism-editor.unautomate", "Unautomate") : tr("organism-editor.automate", "Automate"));
        m.addSeparator();
    }
    m.addItem(3, tr("organism-editor.midi-learn", "MIDI Learn"));
    m.addItem(4, tr("organism-editor.clear-midi", "Clear MIDI"), mapped);
    if (host.osc().enabled() || oscMapped) {
        m.addSeparator();
        m.addItem(5, tr("organism-editor.osc-learn", "OSC Learn"), host.osc().enabled());
        m.addItem(6, tr("organism-editor.clear-osc", "Clear OSC"), oscMapped);
    }
    {
        juce::PopupMenu mod, follow, modNode, followNode;
        std::string modOpen, followOpen;
        bool modOpenRouted = false, followOpenRouted = false;
        auto flush = [](juce::PopupMenu& into, juce::PopupMenu& node, std::string& open,
                        bool& openRouted) {
            if (!open.empty())
                into.addSubMenu(juce::String(open.c_str()), node, true, juce::Image(),
                                openRouted, 0);
            node.clear();
            open.clear();
            openRouted = false;
        };
        for (size_t i = 0; i < sources.size(); ++i) {
            const auto& [from, value] = sources[i];
            const bool isParam = isParamSource(value);
            const auto shown = paramSourceName(value);
            if (from == organism && shown == param) continue;
            auto& into = isParam ? follow : mod;
            auto& node = isParam ? followNode : modNode;
            auto& open = isParam ? followOpen : modOpen;
            auto& openRouted = isParam ? followOpenRouted : modOpenRouted;
            if (from != open) { flush(into, node, open, openRouted); open = from; }
            bool routed = false;
            for (const auto& e : host.mod().map().entries())
                if (e.organism == organism && e.param == param
                    && e.source == from && e.value == value)
                    routed = true;
            openRouted = openRouted || routed;
            node.addItem(100 + (int) i, juce::String(shown.c_str()), true, routed);
        }
        flush(mod, modNode, modOpen, modOpenRouted);
        flush(follow, followNode, followOpen, followOpenRouted);
        juce::PopupMenu followTop;
        followTop.addItem(13, juce::String::fromUTF8("Pick a control\xe2\x80\xa6"));
        if (follow.getNumItems() > 0) {
            followTop.addSeparator();
            followTop.addSubMenu(tr("organism-editor.from-a-list", "From a list"), follow,
                                 true, juce::Image(), followMapped, 0);
        }
        m.addSeparator();
        m.addSubMenu(tr("organism-editor.control-with", "Control with"), mod, mod.getNumItems() > 0);
        if (modMapped) m.addItem(12, tr("organism-editor.release-control", "Release control"));
        m.addSeparator();
        m.addSubMenu(tr("organism-editor.follow", "Follow"), followTop);
        if (followMapped) m.addItem(8, tr("organism-editor.stop-following", "Stop following"));
    }
    m.addSeparator();
    m.addItem(9, tr("organism-editor.clear-all-control", "Clear all control"),
              automated || mapped || oscMapped || modMapped || followMapped);
    if (host.canShowParameterControl()) {
        m.addSeparator();
        m.addItem(7, tr("organism-editor.parameter-control", "Parameter Control..."));
    }
    m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({screen.x, screen.y, 1, 1}),
                    [&host, organism, param, automated, locked, lo, hi, onChanged, sources,
                     automateOverride, screen](int r) {
        if (r == 1) {
            if (automateOverride)   automateOverride(!automated);
            else if (automated)     host.automation().remove(organism, param);
            else                    host.automation().add(organism, param);
            if (onChanged) onChanged();
        } else if (r == 14) {
            resetParam(host, organism, param);
        } else if (r == 10) {
            randomizeParam(host, organism, param);
            if (onChanged) onChanged();
        } else if (r == 11) {
            host.setRollLocked(organism, param, !locked);
            if (onChanged) onChanged();
        } else if (r == 3) {
            MidiLearner::instance().arm(host, organism, param, lo, hi);
        } else if (r == 4) {
            std::vector<MidiSource> ccs;
            for (const auto& e : host.midi().map().entries())
                if (e.organism == organism && e.param == param) ccs.push_back(e.source());
            for (const auto& cc : ccs) host.midi().clearCC(cc, organism, param);
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
            host.showParameterControl(organism, param);
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
                std::vector<MidiSource> ccs;
                for (const auto& e : host.midi().map().entries())
                    if (e.organism == organism && e.param == param) ccs.push_back(e.source());
                for (const auto& cc : ccs) host.midi().clearCC(cc, organism, param);
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
            } else if (paramIsSwitch(host, organism, param)) {
                host.mod().mapRoute(sv.first, sv.second, organism, param, lo, hi);
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
