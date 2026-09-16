// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/assistant/AssistantEngine.h"

#include <cmath>

namespace hum {

juce::String AssistantEngine::runTool(const ToolCall& c) {
    const auto s = [](const juce::var& v) { return v.toString().toStdString(); };
    const auto& in = c.input;

    if (c.name == "list_patch") return patchJson();
    if (c.name == "list_classes") return classesJson();
    if (c.name == "describe") return describeJson(s(in["name"]));

    if (c.name == "add") {
        const std::string cls = resolveClass(s(in["class"]));
        if (cls.empty()) return unknownClass(s(in["class"]));
        const std::string pod = s(in["pod"]);
        if (!pod.empty() && !pods::isPod(host_.model(), pod))
            return "error: no pod named '" + juce::String(pod)
                   + "' - pods are the prefixes in node names like 'Pod_1/Kick'";
        const int x = in.hasProperty("x") ? (int) in["x"] : 120 + 40 * (int) host_.model().organisms.size();
        const int y = in.hasProperty("y") ? (int) in["y"] : 120 + 30 * ((int) host_.model().organisms.size() % 8);
        const auto name = host_.addOrganism(cls, {x, y}, pod);
        edited_ = true;
        return name.empty() ? juce::String("error: could not add")
                            : "added '" + juce::String(name) + "'"
                              + (pod.empty() ? juce::String(" on the top canvas")
                                             : " inside '" + juce::String(pod) + "'");
    }
    if (c.name == "remove") {
        if (!host_.model().byName(s(in["name"]))) return unknownNode(in["name"]);
        host_.removeOrganism(s(in["name"]));
        edited_ = true;
        return "removed";
    }
    if (c.name == "rename") {
        if (!host_.model().byName(s(in["name"]))) return unknownNode(in["name"]);
        const bool ok = host_.renameOrganism(s(in["name"]), s(in["new_name"]));
        edited_ |= ok;
        return ok ? "renamed" : "error: name empty or already taken";
    }
    if (c.name == "replace") {
        if (!host_.model().byName(s(in["name"]))) return unknownNode(in["name"]);
        const std::string cls = resolveClass(s(in["class"]));
        if (cls.empty()) return unknownClass(s(in["class"]));
        const auto n = host_.replaceOrganism(s(in["name"]), cls);
        edited_ = true;
        return n.empty() ? juce::String("error: could not replace") : "replaced";
    }
    if (c.name == "connect" || c.name == "disconnect") {
        const std::string src = s(in["src"]), dst = s(in["dst"]);
        if (!host_.model().byName(src)) return unknownNode(in["src"]);
        if (!host_.model().byName(dst)) return unknownNode(in["dst"]);
        const int outlet = (int) in["outlet"], inlet = (int) in["inlet"];
        const bool midi = (bool) in["midi"];
        edited_ = true;
        if (c.name == "connect") {
            if (midi) host_.connectMidi(src, outlet, dst, inlet);
            else      host_.connect(src, outlet, dst, inlet);
            return "connected";
        }
        if (midi) host_.removeMidiConnection(src, outlet, dst, inlet);
        else      host_.removeConnection(src, outlet, dst, inlet);
        return "disconnected";
    }
    if (c.name == "set_param" || c.name == "set_param_text") {
        const std::string node = s(in["name"]);
        const auto* cm = host_.model().byName(node);
        if (!cm) return unknownNode(in["name"]);
        std::string param;
        juce::StringArray all;
        for (const auto& d : schemaFor(cm->classRaw)) {
            all.add(d.name);
            if (juce::String(d.name).equalsIgnoreCase(juce::String(s(in["param"]))))
                param = d.name;
        }
        if (param.empty())
            return "error: '" + juce::String(s(in["param"])) + "' is not a parameter of "
                   + juce::String(cm->displayClass) + " - its params are: "
                   + all.joinIntoString(", ");
        host_.pushUndo();
        edited_ = true;
        if (c.name == "set_param_text") {
            const std::string text = s(in["text"]);
            if (param.rfind("File", 0) == 0 && !fileTextAllowed(text))
                return "error: a file for this box must live under the user's home folder "
                       "or the Humus library";
            host_.setParamText(node, param, text);
            return "set";
        }
        double v = (double) in["value"];
        for (const auto& d : schemaFor(cm->classRaw))
            if (d.name == param && !d.isText) v = juce::jlimit(d.min, d.max, v);
        host_.setParam(node, param, v);
        return "set to " + juce::String(v);
    }
    if (c.name == "set_tempo") {
        host_.pushUndo();
        host_.performTempo((double) in["bpm"]);
        edited_ = true;
        return "tempo set";
    }
    if (c.name == "transport") {
        const auto a = c.input["action"].toString();
        if (a == "play") host_.play();
        else if (a == "stop") host_.stop();
        else if (a == "play_from_start") host_.playFromStart();
        else return "error: unknown action";
        return a + " ok";
    }
    return "error: unknown tool";
}

juce::String AssistantEngine::patchJson() const {
    auto* root = new juce::DynamicObject();
    juce::Array<juce::var> nodes, cords, midiCords;
    for (const auto& cm : host_.model().organisms) {
        auto* n = new juce::DynamicObject();
        n->setProperty("name", juce::String(cm.name));
        n->setProperty("class", juce::String(cm.displayClass));
        const auto p = host_.position(cm.name);
        n->setProperty("x", p.x);
        n->setProperty("y", p.y);
        nodes.add(juce::var(n));
    }
    for (const auto& cn : host_.model().connections) {
        auto* e = new juce::DynamicObject();
        e->setProperty("src", juce::String(cn.src));
        e->setProperty("outlet", cn.srcOutlet);
        e->setProperty("dst", juce::String(cn.dst));
        e->setProperty("inlet", cn.dstInlet);
        cords.add(juce::var(e));
    }
    for (const auto& cn : host_.model().midiConnections) {
        auto* e = new juce::DynamicObject();
        e->setProperty("src", juce::String(cn.src));
        e->setProperty("outlet", cn.srcOutlet);
        e->setProperty("dst", juce::String(cn.dst));
        e->setProperty("inlet", cn.dstInlet);
        midiCords.add(juce::var(e));
    }
    root->setProperty("tempo", host_.tempo());
    root->setProperty("nodes", nodes);
    root->setProperty("connections", cords);
    root->setProperty("midi_connections", midiCords);
    return juce::JSON::toString(juce::var(root), true);
}

void AssistantEngine::measurePeaksAsync(std::function<void(const std::map<std::string, float>&)> done) {
    auto peaks = std::make_shared<std::map<std::string, float>>();
    ticker_.start(kPeakTickMs, kPeakTicks,
                  [this, peaks] {
                      for (const auto& cm : host_.model().organisms) {
                          float pk = 0.0f, midi = 0.0f;
                          host_.nodeActivity(cm.name, pk, midi);
                          auto& mx = (*peaks)[cm.name];
                          mx = std::max(mx, pk);
                      }
                  },
                  [peaks, done] { done(*peaks); });
}

juce::String AssistantEngine::levelsJson(const std::map<std::string, float>& peaksIn) {
    auto peaks = peaksIn;
    juce::Array<juce::var> nodes;
    for (const auto& cm : host_.model().organisms) {
        const float pk = peaks[cm.name];
        auto* n = new juce::DynamicObject();
        n->setProperty("name", juce::String(cm.name));
        n->setProperty("peak", std::round(pk * 1000.0f) / 1000.0);
        n->setProperty("db", pk > 0.0001f
                                 ? std::round(20.0 * std::log10((double) pk) * 10.0) / 10.0
                                 : -80.0);
        nodes.add(juce::var(n));
    }
    auto* root = new juce::DynamicObject();
    root->setProperty("window_ms", 600);
    root->setProperty("note", "linear peak, 1.0 = full scale; the node feeding "
                              "SoundOut at or above 1.0 is clipping");
    root->setProperty("nodes", nodes);
    return juce::JSON::toString(juce::var(root), true);
}

juce::String AssistantEngine::spectrumJsonFromCapture(double sr) {
    constexpr int kOrder = 12, kSize = 1 << kOrder;
    juce::dsp::FFT fft(kOrder);
    std::vector<float> frame((size_t) kSize * 2), window((size_t) kSize),
        mags((size_t) kSize / 2);
    for (int i = 0; i < kSize; ++i)
        window[(size_t) i] = 0.5f
            - 0.5f * std::cos(juce::MathConstants<float>::twoPi * (float) i / (kSize - 1));

    juce::Array<juce::var> nodes;
    std::vector<float> captured;
    for (const auto& cm : host_.model().organisms) {
        const int n = host_.copyNodeCapture(cm.name, captured);
        const float* d = captured.data();
        if (n < kSize) continue;
        std::fill(mags.begin(), mags.end(), 0.0f);
        int frames = 0;
        for (int off = 0; off + kSize <= n; off += kSize / 2) {
            for (int i = 0; i < kSize; ++i) frame[(size_t) i] = d[off + i] * window[(size_t) i];
            std::fill(frame.begin() + kSize, frame.end(), 0.0f);
            fft.performFrequencyOnlyForwardTransform(frame.data());
            for (int i = 0; i < kSize / 2; ++i) mags[(size_t) i] += frame[(size_t) i];
            ++frames;
        }
        if (frames == 0) continue;
        const float scale = 4.0f / ((float) frames * (float) kSize);
        for (auto& m : mags) m *= scale;
        const auto db = spectrum::bandDb(mags.data(), kSize / 2, sr);
        auto* bandsObj = new juce::DynamicObject();
        for (size_t b = 0; b < db.size(); ++b)
            bandsObj->setProperty(spectrum::bands()[b].name, std::round(db[b] * 10.0) / 10.0);
        auto* node = new juce::DynamicObject();
        node->setProperty("name", juce::String(cm.name));
        node->setProperty("bands", juce::var(bandsObj));
        nodes.add(juce::var(node));
    }
    auto* root = new juce::DynamicObject();
    root->setProperty("note", "band RMS in dB (0 = full-scale sine, -80 = silent); "
                              "two nodes loud in the same band are masking each other");
    root->setProperty("nodes", nodes);
    return juce::JSON::toString(juce::var(root), true);
}

juce::String AssistantEngine::classesJson() const {
    auto* root = new juce::DynamicObject();
    for (const auto& cls : Registry::instance().classNames()) {
        if (!PackRegistry::instance().isClassEnabled(cls)) continue;
        const auto* mf = PackRegistry::instance().classManifest(cls);
        const juce::String cat = mf ? juce::String(mf->category) : "Other";
        auto existing = root->getProperty(cat);
        juce::Array<juce::var> list;
        if (existing.isArray()) list = *existing.getArray();
        list.add(juce::String(cls));
        root->setProperty(cat, list);
    }
    return juce::JSON::toString(juce::var(root), true);
}

juce::String AssistantEngine::describeJson(const std::string& nameOrClass) const {
    const auto* cm = host_.model().byName(nameOrClass);
    const std::string cls = cm ? cm->classRaw : nameOrClass;
    const auto& schema = schemaFor(cls);
    if (schema.empty()) return "error: unknown node or class '" + juce::String(nameOrClass) + "'";
    juce::Array<juce::var> params;
    for (const auto& d : schema) {
        auto* p = new juce::DynamicObject();
        p->setProperty("name", juce::String(d.name));
        if (d.isText) {
            p->setProperty("type", "text");
        } else {
            p->setProperty("min", d.min);
            p->setProperty("max", d.max);
            p->setProperty("default", d.def);
            if (d.isBool) p->setProperty("type", "bool");
            if (d.isEnum) p->setProperty("type", "enum");
            if (cm) p->setProperty("current",
                                   const_cast<AssistantHost&>(host_).liveParamValue(nameOrClass, d.name));
        }
        params.add(juce::var(p));
    }
    auto* root = new juce::DynamicObject();
    root->setProperty("class", juce::String(cls));
    root->setProperty("params", params);
    return juce::JSON::toString(juce::var(root), true);
}

}
