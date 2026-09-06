#pragma once
#include <cmath>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <juce_dsp/juce_dsp.h>

#include "core/AppPaths.h"
#include "core/IsfParse.h"
#include "core/SsfShim.h"
#include "core/VideoGraph.h"
#include "gui/SceneAssemble.h"
#include "core/VisualUniforms.h"
#include "gui/EngineHost.h"
#include "gui/VideoDeckPool.h"
#include "gui/VideoLayer.h"
#include "gui/VisualGlCanvas.h"
#include "gui/VisualPlan.h"
#include "hum/Capabilities.h"

namespace hum {

inline bool isVideoOutputNode(EngineHost& host, const std::string& node) {
    auto* org = host.liveOrganism(node);
    if (auto* v = dynamic_cast<VideoNode*>(org))
        return v->numVideoOutputs() == 0 && v->numVideoInputs() > 0
               && dynamic_cast<VisualSource*>(org) == nullptr
               && dynamic_cast<VideoFrameSink*>(org) == nullptr;
    return false;
}

class VisualPlanBuilder {
public:
    VisualPlanBuilder(EngineHost& host, std::string node, bool isOutput)
        : host_(host), name_(std::move(node)), isOutput_(isOutput) {}

    explicit VisualPlanBuilder(EngineHost& host) : host_(host), isOutput_(false) {}

    ~VisualPlanBuilder() {
        for (const auto& n : tapped_)
            if (auto* vs = dynamic_cast<VisualSource*>(host_.liveOrganism(n)))
                vs->setVisualTapEnabled(false);
    }

    struct Want {
        std::string node;
        bool isOutput = true;
        int w = 0, h = 0;
        bool everyOther = false;
    };

    visual::Plan build() {
        auto p = buildAll({{name_, isOutput_, 0, 0, false}});
        p.root = p.taps[0].step;
        p.masterFade = p.taps[0].fade;
        if (host_.bypassed(name_)) p.noSignal = true;
        p.taps.clear();
        return p;
    }

    visual::Plan buildAll(const std::vector<Want>& wants) {
        const float dt = 1.0f / 30.0f;
        if (++scenePoll_ >= 30) scenePoll_ = 0;
        const bool pollMtime = scenePoll_ == 0;

        visual::Plan p;

        std::vector<std::string> order, roots;
        std::set<std::string> seen;
        for (const auto& want : wants) {
            roots.push_back(want.isOutput ? host_.videoSourceInto(want.node, 0)
                                          : want.node);
            if (roots.back().empty()) continue;
            for (const auto& n :
                 videoRenderOrder(roots.back(), host_.model().videoConnections))
                if (seen.insert(n).second) order.push_back(n);
        }
        if (!roots.empty()) root_ = roots.front();
        if ((int) order.size() > visual::kMaxSteps) order.resize(visual::kMaxSteps);

        std::map<std::string, int> stepOf;
        std::set<std::string> live;
        for (const auto& node : order) {
            visual::Step s;
            s.node = node;
            auto* org = host_.liveOrganism(node);
            auto* vn = dynamic_cast<VideoNode*>(org);
            if (host_.bypassed(node)) {
                s.kind = visual::Step::Mix;
                const auto src = host_.videoSourceInto(node, 0);
                const auto it = stepOf.find(src);
                s.mixA = (!src.empty() && it != stepOf.end()) ? it->second : -1;
                s.mixB = -1;
                s.mixFade = 0.0f;
                live.insert(node);
                stepOf[node] = (int) p.steps.size();
                p.steps.push_back(std::move(s));
                continue;
            }
            if (auto* vs = dynamic_cast<VisualSource*>(org)) {
                vs->setVisualTapEnabled(true);
                tapped_.insert(node);
                s.kind = visual::Step::Scene;
                buildSceneStep(node, s, dt, pollMtime);
                const int ins = vn != nullptr ? vn->numVideoInputs() : 0;
                for (int i = 0; i < ins; ++i) {
                    const auto src = host_.videoSourceInto(node, i);
                    const auto it = stepOf.find(src);
                    if (src.empty() || it == stepOf.end()) continue;
                    visual::LayerIn l;
                    l.src = it->second;
                    l.opacity = paramOr(src, "Opacity", 1.0f);
                    l.blend = (int) paramOr(src, "Blend", 0.0f);
                    s.layers.push_back(l);
                }
            } else if (vn != nullptr && vn->numVideoInputs() == 0
                       && vn->numVideoOutputs() > 0) {
                s.kind = visual::Step::Deck;
                buildDeckStep(node, s);
                live.insert(node);
            } else if (vn != nullptr && vn->numVideoInputs() == 1
                       && vn->numVideoOutputs() == 1) {
                s.kind = visual::Step::Mix;
                const auto src = host_.videoSourceInto(node, 0);
                const auto it = stepOf.find(src);
                s.mixA = (!src.empty() && it != stepOf.end()) ? it->second : -1;
                s.mixB = -1;
                s.mixFade = 0.0f;
            } else if (vn != nullptr && vn->numVideoInputs() >= 2
                       && vn->numVideoOutputs() > 0) {
                s.kind = visual::Step::Mix;
                const auto a = host_.videoSourceInto(node, 0);
                const auto b = host_.videoSourceInto(node, 1);
                const auto ia = stepOf.find(a), ib = stepOf.find(b);
                s.mixA = ia != stepOf.end() ? ia->second : -1;
                s.mixB = ib != stepOf.end() ? ib->second : -1;
                s.mixFade = paramOr(node, "Fade", 0.0f);
            }
            stepOf[node] = (int) p.steps.size();
            p.steps.push_back(std::move(s));
        }
        for (size_t i = 0; i < wants.size(); ++i) {
            visual::Tap t;
            t.node = wants[i].node;
            const auto it = stepOf.find(roots[i]);
            t.step = !roots[i].empty() && it != stepOf.end() ? it->second : -1;
            if (host_.bypassed(wants[i].node)) t.step = -1;
            t.w = wants[i].w;
            t.h = wants[i].h;
            t.fade = wants[i].isOutput ? paramOr(wants[i].node, "Fade", 1.0f) : 1.0f;
            t.everyOther = wants[i].everyOther;
            p.taps.push_back(std::move(t));
        }

        for (auto it = decks_.begin(); it != decks_.end();)
            it = live.count(it->first) ? std::next(it) : decks_.erase(it);
        for (auto it = scenes_.begin(); it != scenes_.end();)
            it = stepOf.count(it->first) ? std::next(it) : scenes_.erase(it);

        return p;
    }

    const std::string& root() const { return root_; }

    juce::String parseError() const {
        for (const auto& [node, st] : scenes_)
            if (st.parseError.isNotEmpty())
                return juce::String(node) + ": " + st.parseError;
        return {};
    }

    float paramOr(const std::string& node, const char* param, float def) {
        if (auto* c = host_.liveOrganism(node))
            if (c->params.byName(param) != nullptr)
                return (float) host_.liveParamValue(node, param);
        return def;
    }

private:
    struct SceneState {
        juce::String path;
        juce::Time mtime;
        visual::SceneSpec spec;
        std::vector<isf::Input> isfInputs;
        std::vector<ssf::Control> ssfControls;
        bool ssf = false;
        bool shk = false;
        juce::String parseError;
        float prevBand[4] = {};
        float hits[4] = {};
        visual::Bands bands;
        float time = 0.0f;
        int frameIndex = 0;
    };
    struct DeckState {
        std::shared_ptr<VideoLayer> layer;
        juce::String path;
        float lastRate = 1.0e9f;
        unsigned lastTrig = 0;
        bool trigSeen = false, hadFrame = false;
        std::shared_ptr<const VideoLayer::Frame> camFrame;
        unsigned camGen = 0;
        bool camSeen = false;
    };

    juce::String paramText(const std::string& node, const juce::String& param) const {
        if (const auto* cm = host_.model().byName(node))
            for (const auto& p : cm->properties)
                if (p.name == param.toStdString()) return juce::String(p.text);
        return {};
    }

    juce::File resolvePath(const juce::String& path) const {
        return VideoDeckPool::resolveTape(host_.documentPath(), path);
    }

    void adoptScene(SceneState& st, AssembledScene a) {
        st.parseError = std::move(a.parseError);
        st.isfInputs = std::move(a.isfInputs);
        st.ssfControls = std::move(a.ssfControls);
        st.ssf = a.ssf;
        st.shk = a.shk;
        st.spec = std::move(a.spec);
    }

    void buildSceneStep(const std::string& node, visual::Step& s, float dt, bool pollMtime) {
        auto& st = scenes_[node];

        const auto path = paramText(node, "Scene");
        if (path != st.path || pollMtime) {
            const juce::File f = resolvePath(path);
            juce::Time mtime;
            if (f.isDirectory())
                mtime = juce::Time(juce::jmax(
                    f.getChildFile("main.glsl").getLastModificationTime().toMilliseconds(),
                    f.getChildFile("scene.json").getLastModificationTime().toMilliseconds()));
            else if (f.existsAsFile())
                mtime = f.getLastModificationTime();
            if (path != st.path || mtime != st.mtime) {
                st.path = path;
                st.mtime = mtime;
                adoptScene(st, f.exists() ? assembleSceneFile(f)
                                          : assembleScene(juce::String()));
            }
        }
        s.scene = st.spec;

        visual::Bands next;
        if (auto* vs = dynamic_cast<VisualSource*>(host_.liveOrganism(node))) {
            float wave[visual::kFft];
            const int n = vs->readVisualTap(wave, visual::kFft);
            for (int i = n; i < visual::kFft; ++i) wave[i] = 0.0f;
            float fftBuf[visual::kFft * 2] = {};
            for (int i = 0; i < n; ++i) {
                const float hann = 0.5f - 0.5f * std::cos(2.0f * juce::MathConstants<float>::pi
                                                          * (float) i / (float) (visual::kFft - 1));
                fftBuf[i] = wave[i] * hann;
            }
            fft_.performFrequencyOnlyForwardTransform(fftBuf);
            const float scale = 4.0f / (float) visual::kFft;
            for (int i = 0; i < visual::kFft / 2; ++i) fftBuf[i] *= scale;
            next = visual::fold(fftBuf, visual::kFft / 2, host_.sampleRate(), wave, n);
            for (int i = 0; i < visual::kWaveTex; ++i)
                s.wave[i] = 0.5f + 0.5f * wave[visual::kFft - visual::kWaveTex + i];
            for (int i = 0; i < visual::kFftTex; ++i) s.fft[i] = fftBuf[i];
        }
        visual::smooth(st.bands, next, dt, 0.03f, 0.25f);
        s.bands = st.bands;

        const double beat = host_.positionBeats();
        s.beat = (float) beat;
        s.bpm = (float) host_.tempo();
        const double frac = beat - std::floor(beat);
        s.onBeat = (float) std::exp(-frac * 6.0);
        st.time += dt * (float) host_.liveParamValue(node, "Speed");
        s.time = st.time;
        s.timeDelta = dt;
        s.frameIndex = ++st.frameIndex;
        s.brightness = (float) host_.liveParamValue(node, "Brightness");
        for (int k = 0; k < 4; ++k)
            s.knob[k] = (float) host_.liveParamValue(node, "Knob" + std::to_string(k + 1));
        {
            const auto now = juce::Time::getCurrentTime();
            s.date[0] = (float) now.getYear();
            s.date[1] = (float) (now.getMonth() + 1);
            s.date[2] = (float) now.getDayOfMonth();
            s.date[3] = (float) (now.getHours() * 3600 + now.getMinutes() * 60
                                 + now.getSeconds());
        }

        if (st.ssf) {
            const auto push1 = [&s](const char* n, float v) {
                visual::UniformValue u;
                juce::String(n).copyToUTF8(u.name, sizeof(u.name));
                u.comps = 1; u.v[0] = v;
                s.extra.push_back(u);
            };
            const auto* b = s.bands.band;
            push1("syn_Time", s.time);
            push1("syn_BPM", s.bpm);
            push1("syn_BeatTime", s.beat);
            push1("syn_OnBeat", s.onBeat);
            push1("syn_Level", s.bands.level);
            push1("syn_BassLevel", 0.5f * (b[0] + b[1]));
            push1("syn_MidLevel", 0.5f * (b[2] + b[3]));
            push1("syn_MidHighLevel", 0.5f * (b[4] + b[5]));
            push1("syn_HighLevel", 0.5f * (b[6] + b[7]));
            push1("syn_Intensity", s.bands.level);
            push1("syn_Presence", b[5]);
            const float lv[4] = {0.5f * (b[0] + b[1]), 0.5f * (b[2] + b[3]),
                                 0.5f * (b[4] + b[5]), 0.5f * (b[6] + b[7])};
            for (int k = 0; k < 4; ++k) {
                st.hits[k] = juce::jmax(st.hits[k] * std::exp(-dt * 8.0f),
                                        juce::jlimit(0.0f, 1.0f,
                                                     (lv[k] - st.prevBand[k]) * 12.0f));
                st.prevBand[k] = lv[k];
            }
            push1("syn_BassHits", st.hits[0]);
            push1("syn_MidHits", st.hits[1]);
            push1("syn_MidHighHits", st.hits[2]);
            push1("syn_HighHits", st.hits[3]);
            push1("syn_Hits", juce::jmax(juce::jmax(st.hits[0], st.hits[1]),
                                         juce::jmax(st.hits[2], st.hits[3])));
            push1("syn_BassPresence", lv[0]);
            push1("syn_MidPresence", lv[1]);
            push1("syn_MidHighPresence", lv[2]);
            push1("syn_HighPresence", lv[3]);
            const float beatIdx = std::floor(s.beat);
            push1("syn_RandomOnBeat",
                  std::abs(std::sin(beatIdx * 12.9898f) * 43758.5453f)
                      - std::floor(std::abs(std::sin(beatIdx * 12.9898f) * 43758.5453f)));
            push1("syn_ToggleOnBeat", std::fmod(beatIdx, 2.0f));
            push1("syn_FadeInOut", 1.0f);
            const float tau = juce::MathConstants<float>::twoPi;
            push1("syn_BPMSin", 0.5f + 0.5f * std::sin(s.beat * tau));
            push1("syn_BPMSin2", 0.5f + 0.5f * std::sin(s.beat * tau * 0.5f));
            push1("syn_BPMSin4", 0.5f + 0.5f * std::sin(s.beat * tau * 0.25f));
            push1("syn_BPMTri", std::abs(std::fmod(s.beat, 2.0f) - 1.0f));
            push1("syn_BPMTri2", std::abs(std::fmod(s.beat * 0.5f, 2.0f) - 1.0f));
            push1("syn_BPMTri4", std::abs(std::fmod(s.beat * 0.25f, 2.0f) - 1.0f));
            push1("syn_BPMTwitcher", s.onBeat);
            for (const auto& c : st.ssfControls) {
                visual::UniformValue u;
                c.name.copyToUTF8(u.name, sizeof(u.name));
                u.comps = c.comps;
                for (int i = 0; i < 4; ++i) u.v[i] = c.def[i];
                s.extra.push_back(u);
            }
        }

        if (st.shk) {
            const auto push = [&s](const char* n, int comps, float a, float bb = 0.0f,
                                   float c = 0.0f, float d = 0.0f) {
                visual::UniformValue u;
                juce::String(n).copyToUTF8(u.name, sizeof(u.name));
                u.comps = comps; u.v[0] = a; u.v[1] = bb; u.v[2] = c; u.v[3] = d;
                s.extra.push_back(u);
            };
            const float k1 = s.knob[0], k2 = s.knob[1], k3 = s.knob[2], k4 = s.knob[3];
            push("u_time", 1, s.time);
            push("u_strength", 1, k1 * 10.0f);
            push("u_speed", 1, k2 * 5.0f);
            push("u_frequency", 1, k3 * 30.0f);
            push("u_density", 1, k3 * 16.0f);
            push("u_width", 1, 1.0f + k4 * 9.0f);
            push("u_brightness", 1, s.brightness * 3.0f);
            push("u_rows", 1, std::floor(2.0f + k3 * 18.0f));
            push("u_cols", 1, std::floor(2.0f + k3 * 18.0f));
            push("u_red", 1, k4);
            push("u_group_size", 1, 1.0f + k4 * 7.0f);
            push("u_center", 2, 0.5f, 0.5f);
            const auto c1 = juce::Colour::fromHSV(k2, 0.8f, 1.0f, 1.0f);
            const auto c2 = juce::Colour::fromHSV(std::fmod(k2 + 0.33f, 1.0f), 0.8f, 1.0f, 1.0f);
            push("u_color", 4, c1.getFloatRed(), c1.getFloatGreen(), c1.getFloatBlue(), 1.0f);
            push("u_first_color", 4, c1.getFloatRed(), c1.getFloatGreen(), c1.getFloatBlue(), 1.0f);
            push("u_second_color", 4, c2.getFloatRed(), c2.getFloatGreen(), c2.getFloatBlue(), 1.0f);
        }

        int floatSlot = 0;
        for (const auto& in : st.isfInputs) {
            visual::UniformValue u;
            in.name.copyToUTF8(u.name, sizeof(u.name));
            switch (in.type) {
                case isf::InputType::Float: {
                    float v = in.def[0];
                    if (floatSlot < 4) {
                        const float k = s.knob[floatSlot++];
                        v = in.hasRange ? (float) (in.min + k * (in.max - in.min)) : k;
                    }
                    u.comps = 1; u.v[0] = v;
                    break;
                }
                case isf::InputType::Bool:
                case isf::InputType::Event:
                case isf::InputType::Long:
                    u.comps = 1; u.isInt = true; u.v[0] = in.def[0];
                    break;
                case isf::InputType::Color:
                    u.comps = 4;
                    for (int c = 0; c < 4; ++c) u.v[c] = in.def[c];
                    break;
                case isf::InputType::Point2D:
                    u.comps = 2; u.v[0] = in.def[0]; u.v[1] = in.def[1];
                    break;
            }
            s.extra.push_back(u);
        }

        s.sceneOpacity = paramOr(node, "SceneOpacity", 1.0f);
        s.sceneBlend = (int) paramOr(node, "SceneBlend", 0.0f);
    }

    void buildCameraStep(CamPreviewSource& cam, DeckState& d, visual::Step& s) {
        const unsigned gen = cam.camGeneration();
        if (gen != d.camGen || !d.camSeen) {
            d.camGen = gen;
            d.camSeen = true;
            const auto src = cam.camFrame();
            if (src.width > 0 && src.height > 0
                && src.rgba.size() >= (size_t) src.width * (size_t) src.height * 4) {
                auto f = std::make_shared<VideoLayer::Frame>();
                f->width = src.width;
                f->height = src.height;
                f->bgra.resize(src.rgba.size());
                const size_t px = (size_t) src.width * (size_t) src.height;
                for (size_t i = 0; i < px; ++i) {
                    const auto* in = src.rgba.data() + i * 4;
                    auto* out = f->bgra.data() + i * 4;
                    out[0] = in[2]; out[1] = in[1]; out[2] = in[0]; out[3] = 255;
                }
                d.camFrame = std::move(f);
                d.hadFrame = true;
            }
            s.frame = d.camFrame;
        }
        s.active = d.hadFrame && cam.camActive();
    }

    void buildDeckStep(const std::string& node, visual::Step& s) {
        auto& d = decks_[node];
        if (auto* cam = dynamic_cast<CamPreviewSource*>(host_.liveOrganism(node)))
            return buildCameraStep(*cam, d, s);
        const auto path = paramText(node, "File");
        if (path != d.path) {
            d.path = path;
            d.hadFrame = false;
            const auto file = resolvePath(path);
            d.layer = path.isEmpty() || file == juce::File()
                          ? nullptr
                          : VideoDeckPool::instance().open(node, file.getFullPathName());
        }
        if (d.layer == nullptr) return;
        const auto rate = paramOr(node, "Rate", 1.0f);
        if (std::abs(rate - d.lastRate) > 1.0e-3f) {
            d.lastRate = rate;
            d.layer->setRate(rate);
        }
        if (auto* vn = dynamic_cast<VideoNode*>(host_.liveOrganism(node))) {
            const unsigned t = vn->videoLaunchCount();
            if (!d.trigSeen) { d.trigSeen = true; d.lastTrig = t; }
            else if (t != d.lastTrig) { d.lastTrig = t; d.layer->restart(); }
        }
        auto f = d.layer->latestFrame();
        if (f != nullptr) d.hadFrame = true;
        s.frame = std::move(f);
        s.active = d.hadFrame;
    }

    EngineHost& host_;
    std::string name_;
    const bool isOutput_;
    std::string root_;
    juce::dsp::FFT fft_{visual::kFftOrder};
    int scenePoll_ = 0;
    std::map<std::string, SceneState> scenes_;
    std::map<std::string, DeckState> decks_;
    std::set<std::string> tapped_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VisualPlanBuilder)
};

}
