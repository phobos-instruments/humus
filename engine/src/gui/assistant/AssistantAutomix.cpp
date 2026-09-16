// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/assistant/AssistantEngine.h"

#include <cmath>

namespace hum {

juce::String AssistantEngine::levelParamOf(const std::string& cls) {
    static const char* names[] = {"Gain", "Level", "Volume", "Output", "Amp"};
    for (const auto& d : schemaFor(cls)) {
        if (d.isText || d.isBool || d.isEnum) continue;
        for (const auto* nm : names)
            if (juce::String(d.name).equalsIgnoreCase(nm)) return d.name;
    }
    return {};
}

void AssistantEngine::paramRange(const std::string& cls, const juce::String& param, double& mn, double& mx) {
    for (const auto& d : schemaFor(cls))
        if (juce::String(d.name) == param) { mn = d.min; mx = d.max; return; }
}

void AssistantEngine::applyMove(const std::string& node, const juce::String& param, double from, double to, juce::Array<juce::var>& moved) {
    host_.pushUndo();
    host_.setParam(node, param.toStdString(), to);
    edited_ = true;
    auto* m = new juce::DynamicObject();
    m->setProperty("node", juce::String(node));
    m->setProperty("param", param);
    m->setProperty("from", std::round(from * 1000.0) / 1000.0);
    m->setProperty("to", std::round(to * 1000.0) / 1000.0);
    moved.add(juce::var(m));
}

AssistantEngine::AutomixStage AssistantEngine::automixStage(const std::map<std::string, float>& peaksIn) {
    AutomixStage st;
    auto peaks = peaksIn;
    const auto& model = host_.model();
    const auto hasAudioIn = [&](const std::string& n) {
        for (const auto& c : model.connections)
            if (c.dst == n) return true;
        return false;
    };

    std::vector<automix::Source> sources;
    std::map<std::string, juce::String> paramOf;
    auto& skipped = st.skipped;
    std::string soundOut;
    for (const auto& cm : model.organisms) {
        if (classHasRole(cm.classRaw, role::kMasterOut) && soundOut.empty()) soundOut = cm.name;
        if (hasAudioIn(cm.name) || host_.outletsOf(cm.name) <= 0) continue;
        const auto param = levelParamOf(cm.classRaw);
        if (param.isEmpty()) {
            if (peaks[cm.name] >= (float) automix::kSilentPeak)
                skipped.add(juce::String(cm.name) + " (no level param - an SGain "
                                                    "trim after it would fix that)");
            continue;
        }
        double mn = 0.0, mx = 1.0;
        paramRange(cm.classRaw, param, mn, mx);
        paramOf[cm.name] = param;
        sources.push_back({cm.name, classHasRole(cm.classRaw, role::kMixAnchor), (double) peaks[cm.name],
                           host_.liveParamValue(cm.name, param.toStdString()), mn, mx});
    }
    const size_t anchor = automix::pickAnchor(sources);
    if (anchor == (size_t) -1) {
        st.error = "error: no audible sources to stage - is the patch making sound?";
        return st;
    }

    for (const auto& mv : automix::plan(sources, anchor))
        applyMove(mv.name, paramOf[mv.name], mv.from, mv.to, st.moved);

    for (const auto& c : model.connections)
        if (c.dst == soundOut) { st.master = c.src; break; }
    st.anchor = sources[anchor].name;
    return st;
}

juce::String AssistantEngine::automixFinish(AutomixStage st, const std::map<std::string, float>& peaks) {
    const auto& model = host_.model();
    auto& moved = st.moved;
    const auto& skipped = st.skipped;
    const std::string& master = st.master;
    juce::String masterNote = "no master gain staged";
    if (const auto* mcm = model.byName(master)) {
        const auto param = levelParamOf(mcm->classRaw);
        const auto it = peaks.find(master);
        const double mpk = it != peaks.end() ? it->second : 0.0;
        const double cur = param.isNotEmpty()
                               ? host_.liveParamValue(master, param.toStdString()) : 0.0;
        if (param.isNotEmpty() && mpk > automix::kSilentPeak && cur > 0.0) {
            double mn = 0.0, mx = 1.0;
            paramRange(mcm->classRaw, param, mn, mx);
            applyMove(master, param,
                      cur, juce::jlimit(mn, mx, cur * automix::kMasterTarget / mpk), moved);
            masterNote = juce::String(master) + " staged to a "
                         + juce::String(automix::kMasterTarget, 1) + " peak";
        }
    }

    auto* root = new juce::DynamicObject();
    root->setProperty("anchor", juce::String(st.anchor));
    root->setProperty("moves", moved);
    root->setProperty("master", masterNote);
    if (!skipped.isEmpty()) root->setProperty("skipped", skipped.joinIntoString("; "));
    root->setProperty("note", "gain staging only - use spectrum to find masking and "
                              "levels to verify the result");
    return juce::JSON::toString(juce::var(root), true);
}

}
