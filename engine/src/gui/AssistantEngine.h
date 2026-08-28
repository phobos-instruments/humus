#pragma once
#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <juce_core/juce_core.h>

#include <juce_dsp/juce_dsp.h>

#include "core/AssistantProtocol.h"
#include "core/AutomixPlan.h"
#include "core/ParamSchema.h"
#include "core/PodModel.h"
#include "core/PackRegistry.h"
#include "core/SpectrumBands.h"
#include "gui/AiClient.h"
#include "gui/EngineHost.h"
#include "hum/Registry.h"

namespace hum {

class AssistantEngine {
public:
    explicit AssistantEngine(EngineHost& host) : host_(host) {
        messages_ = juce::Array<juce::var>();
    }
    ~AssistantEngine() {
        *alive_ = false;
        if (busy_) host_.endTransaction();
    }

    std::function<void(const juce::String& role, const juce::String& text)> onLine;
    std::function<void(bool)> onBusy;
    std::function<void()> onPatchEdited;

    bool busy() const { return busy_; }
    void resetConversation() { messages_ = juce::Array<juce::var>(); }

    void send(const juce::String& userText) {
        if (busy_ || userText.trim().isEmpty()) return;
        line("you", userText);
        messages_.append(userMessage(userText));
        rounds_ = 0;
        edited_ = false;
        nudged_ = false;
        setBusy(true);
        host_.beginTransaction();
        continueLoop();
    }

private:
    static constexpr int kMaxRounds = 12;

    void line(const juce::String& role, const juce::String& text) {
        if (onLine && text.isNotEmpty()) onLine(role, text);
    }
    void setBusy(bool b) {
        busy_ = b;
        if (onBusy) onBusy(b);
    }
    void finishTurn() {
        host_.endTransaction();
        setBusy(false);
        if (edited_ && onPatchEdited) onPatchEdited();
    }

    void continueLoop() {
        AiClient::ChatRequest req;
        req.system = assistantSystemPrompt();
        req.messages = messages_;
        req.tools = assistantToolsSpec();
        AiClient::chat(std::move(req),
                           [this, alive = alive_](juce::var response, juce::String error) {
            if (!*alive) return;
            if (error.isNotEmpty()) { line("error", error); finishTurn(); return; }
            messages_.append(assistantMessage(response));
            line("assistant", extractText(response));

            const auto calls = extractToolCalls(response);
            if (calls.empty() || !wantsTools(response)) {
                const auto text = extractText(response);
                const bool phantomProse = !edited_ && claimsEdits(text);
                if (!nudged_ && (phantomProse || looksLikePhantomEdits(text))) {
                    nudged_ = true;
                    line("error", phantomProse
                        ? "That reply claims edits, but no editing tool ran - "
                          "nothing changed. Asking the model to do it for real."
                        : "That reply only pretended to edit (code instead of "
                          "tool calls) - asking the model to do it for real.");
                    messages_.append(userMessage(phantomProse ? editClaimNudge()
                                                              : toolNudge()));
                    continueLoop();
                    return;
                }
                if (phantomProse)
                    line("error", "Heads up: nothing was actually changed this turn.");
                finishTurn();
                return;
            }
            if (++rounds_ > kMaxRounds) {
                line("error", "Stopped: too many tool rounds for one request.");
                finishTurn();
                return;
            }
            std::vector<std::pair<juce::String, juce::String>> results;
            for (const auto& c : calls) {
                line("tool", progressLine(c));
                const juce::String result = runTool(c);
                if (result.startsWith("error")) line("error", result);
                results.emplace_back(c.id, result);
            }
            messages_.append(toolResultsMessage(results));
            continueLoop();
        });
    }

    juce::String runTool(const ToolCall& c) {
        const auto s = [](const juce::var& v) { return v.toString().toStdString(); };
        const auto& in = c.input;

        if (c.name == "list_patch") return patchJson();
        if (c.name == "list_classes") return classesJson();
        if (c.name == "describe") return describeJson(s(in["name"]));
        if (c.name == "levels") return levelsJson();
        if (c.name == "spectrum") return spectrumJson();
        if (c.name == "automix") return automixJson();

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
                host_.setParamText(node, param, s(in["text"]));
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
            host_.setTempo((double) in["bpm"]);
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

    static juce::String unknownNode(const juce::var& n) {
        return "error: no node named '" + n.toString() + "' (see list_patch)";
    }

    static std::string resolveClass(const std::string& cls) {
        std::string ci;
        for (const auto& n : Registry::instance().classNames()) {
            if (n == cls) return n;
            if (ci.empty() && juce::String(n).equalsIgnoreCase(juce::String(cls))) ci = n;
        }
        return ci;
    }

    static juce::String unknownClass(const std::string& cls) {
        juce::StringArray close;
        const auto needle = juce::String(cls).toLowerCase();
        for (const auto& n : Registry::instance().classNames()) {
            const auto hay = juce::String(n).toLowerCase();
            if (hay.contains(needle) || needle.contains(hay)) close.add(n);
            if (close.size() >= 5) break;
        }
        return "error: unknown class '" + juce::String(cls) + "'"
               + (close.isEmpty() ? juce::String(" (see list_classes)")
                                  : " - closest classes: " + close.joinIntoString(", "));
    }

    juce::String patchJson() const {
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

    static constexpr const char* kNotPlaying =
        "error: the transport is stopped - start playback (transport tool) first";

    std::map<std::string, float> measurePeaks() {
        std::map<std::string, float> peaks;
        for (int i = 0; i < 24; ++i) {
            for (const auto& cm : host_.model().organisms) {
                float pk = 0.0f, midi = 0.0f;
                host_.nodeActivity(cm.name, pk, midi);
                auto& mx = peaks[cm.name];
                mx = std::max(mx, pk);
            }
            juce::Thread::sleep(25);
        }
        return peaks;
    }

    juce::String levelsJson() {
        if (!host_.isPlaying()) return kNotPlaying;
        auto peaks = measurePeaks();
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

    juce::String spectrumJson() {
        if (!host_.isPlaying()) return kNotPlaying;
        const double sr = host_.sampleRate();
        if (!host_.armNodeCapture((int) (sr * 0.7)))
            return "error: no live audio engine";
        juce::Thread::sleep(900);
        host_.disarmNodeCapture();

        constexpr int kOrder = 12, kSize = 1 << kOrder;
        juce::dsp::FFT fft(kOrder);
        std::vector<float> frame((size_t) kSize * 2), window((size_t) kSize),
            mags((size_t) kSize / 2);
        for (int i = 0; i < kSize; ++i)
            window[(size_t) i] = 0.5f
                - 0.5f * std::cos(juce::MathConstants<float>::twoPi * (float) i / (kSize - 1));

        juce::Array<juce::var> nodes;
        for (const auto& cm : host_.model().organisms) {
            const int n = host_.nodeCaptureFill(cm.name);
            const float* d = host_.nodeCaptureData(cm.name);
            if (d == nullptr || n < kSize) continue;
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

    juce::String automixJson() {
        if (!host_.isPlaying()) return kNotPlaying;
        const auto& model = host_.model();

        const auto levelParamOf = [this](const std::string& cls) -> juce::String {
            static const char* names[] = {"Gain", "Level", "Volume", "Output", "Amp"};
            for (const auto& d : schemaFor(cls)) {
                if (d.isText || d.isBool || d.isEnum) continue;
                for (const auto* nm : names)
                    if (juce::String(d.name).equalsIgnoreCase(nm)) return d.name;
            }
            return {};
        };
        const auto paramRange = [this](const std::string& cls, const juce::String& param,
                                       double& mn, double& mx) {
            for (const auto& d : schemaFor(cls))
                if (juce::String(d.name) == param) { mn = d.min; mx = d.max; return; }
        };
        const auto hasAudioIn = [&](const std::string& n) {
            for (const auto& c : model.connections)
                if (c.dst == n) return true;
            return false;
        };

        auto peaks = measurePeaks();

        std::vector<automix::Source> sources;
        std::map<std::string, juce::String> paramOf;
        juce::StringArray skipped;
        std::string soundOut;
        for (const auto& cm : model.organisms) {
            if (cm.displayClass == "SoundOut" && soundOut.empty()) soundOut = cm.name;
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
            sources.push_back({cm.name, cm.displayClass, (double) peaks[cm.name],
                               host_.liveParamValue(cm.name, param.toStdString()), mn, mx});
        }
        const size_t anchor = automix::pickAnchor(sources);
        if (anchor == (size_t) -1)
            return "error: no audible sources to stage - is the patch making sound?";

        juce::Array<juce::var> moved;
        const auto apply = [&](const std::string& node, const juce::String& param,
                               double from, double to) {
            host_.pushUndo();
            host_.setParam(node, param.toStdString(), to);
            edited_ = true;
            auto* m = new juce::DynamicObject();
            m->setProperty("node", juce::String(node));
            m->setProperty("param", param);
            m->setProperty("from", std::round(from * 1000.0) / 1000.0);
            m->setProperty("to", std::round(to * 1000.0) / 1000.0);
            moved.add(juce::var(m));
        };
        for (const auto& mv : automix::plan(sources, anchor))
            apply(mv.name, paramOf[mv.name], mv.from, mv.to);

        std::string master;
        for (const auto& c : model.connections)
            if (c.dst == soundOut) { master = c.src; break; }
        juce::String masterNote = "no master gain staged";
        if (const auto* mcm = model.byName(master)) {
            const auto param = levelParamOf(mcm->classRaw);
            const double mpk = measurePeaks()[master];
            const double cur = param.isNotEmpty()
                                   ? host_.liveParamValue(master, param.toStdString()) : 0.0;
            if (param.isNotEmpty() && mpk > automix::kSilentPeak && cur > 0.0) {
                double mn = 0.0, mx = 1.0;
                paramRange(mcm->classRaw, param, mn, mx);
                apply(master, param,
                      cur, juce::jlimit(mn, mx, cur * automix::kMasterTarget / mpk));
                masterNote = juce::String(master) + " staged to a "
                             + juce::String(automix::kMasterTarget, 1) + " peak";
            }
        }

        auto* root = new juce::DynamicObject();
        root->setProperty("anchor", juce::String(sources[anchor].name));
        root->setProperty("moves", moved);
        root->setProperty("master", masterNote);
        if (!skipped.isEmpty()) root->setProperty("skipped", skipped.joinIntoString("; "));
        root->setProperty("note", "gain staging only - use spectrum to find masking and "
                                  "levels to verify the result");
        return juce::JSON::toString(juce::var(root), true);
    }

    juce::String classesJson() const {
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

    juce::String describeJson(const std::string& nameOrClass) const {
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
                                       const_cast<EngineHost&>(host_).liveParamValue(nameOrClass, d.name));
            }
            params.add(juce::var(p));
        }
        auto* root = new juce::DynamicObject();
        root->setProperty("class", juce::String(cls));
        root->setProperty("params", params);
        return juce::JSON::toString(juce::var(root), true);
    }

    EngineHost& host_;
    juce::var messages_;
    int rounds_ = 0;
    bool busy_ = false;
    bool edited_ = false;
    bool nudged_ = false;
    std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);
};

}
