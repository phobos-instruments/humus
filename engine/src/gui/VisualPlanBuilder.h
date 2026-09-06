#pragma once
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstdint>
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
#include "gui/VideoTakeSink.h"
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
        std::shared_ptr<VideoTakeSink> sink;
    };

    void evaluateAt(double beat, double tempo, float frameDelta = 1.0f / 30.0f) {
        atBeat_ = beat;
        atTempo_ = tempo > 0.0 ? tempo : 120.0;
        frameDelta_ = frameDelta > 0.0f ? frameDelta : 1.0f / 30.0f;
        addressed_ = true;
    }
    void evaluateLive() { addressed_ = false; }
    void setExactFrames(bool on) {
        exact_ = on;
        poolPrefix_ = on ? "offline" + std::to_string((std::uintptr_t) this) + "/" : std::string();
    }
    const std::string& poolPrefix() const { return poolPrefix_; }

    visual::Plan buildAt(double beat, double tempo, float frameDelta = 1.0f / 30.0f) {
        evaluateAt(beat, tempo, frameDelta);
        auto p = build();
        evaluateLive();
        return p;
    }

    visual::Plan build() {
        auto p = buildAll({{name_, isOutput_, 0, 0, false}});
        p.root = p.taps[0].step;
        p.masterFade = p.taps[0].fade;
        if (host_.bypassed(name_)) p.noSignal = true;
        p.taps.clear();
        return p;
    }

    visual::Plan buildAll(const std::vector<Want>& wants) {
        const float dt = addressed_ ? frameDelta_ : 1.0f / 30.0f;
        if (++scenePoll_ >= 30) scenePoll_ = 0;
        const bool pollMtime = scenePoll_ == 0;

        visual::Plan p;
        p.beat = addressed_ ? atBeat_ : host_.positionBeats();
        p.tempo = addressed_ ? atTempo_ : host_.tempo();
        p.rolling = addressed_ ? false : host_.isPlaying();

        std::vector<std::string> order, roots;
        std::set<std::string> seen;
        for (const auto& want : wants) {
            roots.push_back(want.isOutput ? host_.videoSourceInto(want.node, 0)
                                          : want.node);
            if (roots.back().empty()) continue;
            std::vector<std::string> starts{roots.back()};
            if (want.isOutput)
                for (const auto& c : host_.model().videoConnections)
                    if (c.dst == want.node && c.dstInlet == 0 && c.src != roots.back())
                        starts.push_back(c.src);
            for (const auto& start : starts)
                for (const auto& n : videoRenderOrder(start, host_.model().videoConnections))
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
                s.mixA = sourceStep(node, 0, stepOf, &p);
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
                    const int step = sourceStep(node, i, stepOf, &p);
                    if (step < 0) continue;
                    visual::LayerIn l;
                    l.src = step;
                    l.opacity = paramOr(src, "Opacity", 1.0f);
                    l.blend = (int) paramOr(src, "Blend", 0.0f);
                    s.layers.push_back(l);
                }
            } else if (auto* clips = dynamic_cast<VideoPadSource*>(org)) {
                s.kind = visual::Step::Mix;
                buildClipsStep(node, *clips, s, p, live, stepOf);
                live.insert(node);
            } else if (auto* track = dynamic_cast<VideoTimelineSource*>(org)) {
                s.kind = visual::Step::Mix;
                const int input = vn != nullptr && vn->numVideoInputs() > 0
                                      ? sourceStep(node, 0, stepOf, &p) : -1;
                buildTrackStep(node, *track, s, p, live,
                               trackShowsInput(node, *track) ? input : -1);
                live.insert(node);
            } else if (vn != nullptr && vn->numVideoInputs() == 0
                       && vn->numVideoOutputs() > 0) {
                s.kind = visual::Step::Deck;
                buildDeckStep(node, s);
                live.insert(node);
            } else if (dynamic_cast<VideoFxNode*>(org) != nullptr) {
                s.kind = visual::Step::Fx;
                s.mixA = sourceStep(node, 0, stepOf, &p);
                buildFxStep(node, s);
            } else if (vn != nullptr && vn->numVideoInputs() == 1
                       && vn->numVideoOutputs() == 1) {
                s.kind = visual::Step::Mix;
                s.mixA = sourceStep(node, 0, stepOf, &p);
                s.mixB = -1;
                s.mixFade = 0.0f;
            } else if (vn != nullptr && vn->numVideoInputs() >= 2
                       && vn->numVideoOutputs() > 0) {
                s.kind = visual::Step::Mix;
                s.mixA = sourceStep(node, 0, stepOf, &p);
                s.mixB = sourceStep(node, 1, stepOf, &p);
                s.mixFade = paramOr(node, "Fade", 0.0f);
                s.mixCurve = paramOr(node, "Curve", 0.0f);
            }
            stepOf[node] = (int) p.steps.size();
            p.steps.push_back(std::move(s));
        }
        for (size_t i = 0; i < wants.size(); ++i) {
            visual::Tap t;
            t.node = wants[i].node;
            const auto it = stepOf.find(roots[i]);
            t.step = wants[i].isOutput ? sourceStep(wants[i].node, 0, stepOf, &p)
                     : !roots[i].empty() && it != stepOf.end() ? it->second : -1;
            if (host_.bypassed(wants[i].node)) t.step = -1;
            t.w = wants[i].w;
            t.h = wants[i].h;
            t.fade = wants[i].isOutput ? paramOr(wants[i].node, "Fade", 1.0f) : 1.0f;
            t.everyOther = wants[i].everyOther;
            t.sink = wants[i].sink;
            p.taps.push_back(std::move(t));
        }

        const double now = juce::Time::getMillisecondCounterHiRes();
        for (auto it = decks_.begin(); it != decks_.end();) {
            if (live.count(it->first)) { ++it; continue; }
            if (it->second.layer != nullptr)
                retired_.push_back({std::move(it->second.layer), now});
            it = decks_.erase(it);
        }
        for (auto it = retired_.begin(); it != retired_.end();)
            it = now - it->at < kRetireGraceMs ? std::next(it) : retired_.erase(it);
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

    static int stepFor(const std::map<std::string, int>& stepOf, const std::string& src,
                       int outlet) {
        const auto it = stepOf.find(outlet > 0 ? src + "#" + std::to_string(outlet) : src);
        return it != stepOf.end() ? it->second : -1;
    }

    int sourceStep(const std::string& node, int inlet, const std::map<std::string, int>& stepOf,
                   visual::Plan* stack = nullptr) {
        int outlet = 0;
        const auto src = host_.videoSourceInto(node, inlet, outlet);
        if (src.empty()) return -1;
        int base = stepFor(stepOf, src, outlet);
        if (stack == nullptr) return base;
        for (const auto& c : host_.model().videoConnections) {
            if (c.dst != node || c.dstInlet != inlet || c.src == src) continue;
            const int over = stepFor(stepOf, c.src, c.srcOutlet);
            if (over < 0) continue;
            visual::Step m;
            m.kind = visual::Step::Mix;
            m.node = node + "+" + c.src;
            m.mixA = base;
            m.mixB = over;
            m.mixFade = juce::jlimit(0.0f, 1.0f, paramOr(c.src, "Opacity", 1.0f));
            m.mixSum = true;
            base = (int) stack->steps.size();
            stack->steps.push_back(std::move(m));
        }
        return base;
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
        double shownPts = -1.0, sourceStep = 0.0;
        VideoLayer::LoopRange range{-1.0, -1.0, true};
        int staged = -1;
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

        const double beat = addressed_ ? atBeat_ : host_.positionBeats();
        s.beat = (float) beat;
        s.bpm = (float) (addressed_ ? atTempo_ : host_.tempo());
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

    void buildFxStep(const std::string& node, visual::Step& s) {
        auto& f = s.fx;
        f.posX = paramOr(node, "PosX", 0.0f);
        f.posY = paramOr(node, "PosY", 0.0f);
        f.scale = paramOr(node, "Scale", 1.0f);
        f.rotate = paramOr(node, "Rotate", 0.0f) * juce::MathConstants<float>::pi / 180.0f;
        f.brightness = paramOr(node, "Brightness", 1.0f);
        f.contrast = paramOr(node, "Contrast", 1.0f);
        f.saturation = paramOr(node, "Saturation", 1.0f);
        f.hue = paramOr(node, "Hue", 0.0f) * juce::MathConstants<float>::pi / 180.0f;
        f.invert = paramOr(node, "Invert", 0.0f) >= 0.5f ? 1.0f : 0.0f;
        f.pixelate = paramOr(node, "Pixelate", 0.0f);
        f.mirror = (int) paramOr(node, "Mirror", 0.0f);
        s.active = true;
    }

    std::shared_ptr<VideoLayer> openDeck(DeckState& d, const std::string& key,
                                         const juce::String& path) {
        if (path != d.path) {
            d.path = path;
            d.hadFrame = false;
            d.shownPts = -1.0;
            d.sourceStep = 0.0;
            const auto file = resolvePath(path);
            d.layer = path.isEmpty() || file == juce::File()
                          ? nullptr
                          : VideoDeckPool::instance().open(poolPrefix_ + key,
                                                           file.getFullPathName(), exact_);
        }
        return d.layer;
    }

    bool padOutletWired(const std::string& node, int slot) const {
        for (const auto& c : host_.model().videoConnections)
            if (c.src == node && c.srcOutlet == slot + 1) return true;
        return false;
    }

    void buildClipsStep(const std::string& node, VideoPadSource& clips, visual::Step& s,
                        visual::Plan& p, std::set<std::string>& live,
                        std::map<std::string, int>& stepOf) {
        const auto st = clips.clipState();
        auto& launch = decks_[node];
        const bool launched = launch.trigSeen && st.launches != launch.lastTrig;
        launch.trigSeen = true;
        launch.lastTrig = st.launches;
        const float rate = paramOr(node, "Rate", 1.0f);
        for (int slot = 0; slot < clips.clipCount(); ++slot) {
            const auto n = std::to_string(slot + 1);
            const auto key = node + "/" + n;
            const auto path = paramText(node, "File" + n);
            if (path.isEmpty()) continue;
            live.insert(key);
            auto& d = decks_[key];
            auto layer = openDeck(d, key, path);
            if (layer == nullptr) continue;
            VideoLayer::LoopRange r;
            r.in = paramOr(node, ("In" + n).c_str(), 0.0f);
            r.out = paramOr(node, ("Out" + n).c_str(), 0.0f);
            r.loop = paramOr(node, ("Loop" + n).c_str(), 1.0f) >= 0.5f;
            if (r.in != d.range.in || r.out != d.range.out || r.loop != d.range.loop) {
                d.range = r;
                layer->setLoopRange(r);
            }
            if (std::abs(rate - d.lastRate) > 1.0e-3f) {
                d.lastRate = rate;
                layer->setRate(rate);
            }
            clips.noteClipLength(slot, layer->lengthSeconds());
            const bool wired = padOutletWired(node, slot);
            const bool onStage = slot == st.active || slot == st.outgoing || wired;
            if (launched && slot == st.active) layer->restart();
            if (d.staged != (onStage ? 1 : 0)) {
                d.staged = onStage ? 1 : 0;
                layer->setPaused(!onStage);
            }
            if (!onStage) continue;
            visual::Step ds;
            ds.kind = visual::Step::Deck;
            ds.node = key;
            auto f = layer->latestFrame();
            if (f != nullptr) d.hadFrame = true;
            ds.frame = std::move(f);
            ds.active = d.hadFrame;
            const int idx = (int) p.steps.size();
            p.steps.push_back(std::move(ds));
            if (wired) stepOf[node + "#" + n] = idx;
            if (slot == st.active) s.mixB = idx;
            else if (slot == st.outgoing) s.mixA = idx;
        }
        if (paramOr(node, "Mute", 0.0f) >= 0.5f) s.mixA = s.mixB = -1;
        s.mixFade = juce::jlimit(0.0f, 1.0f, st.phase);
    }

    static constexpr int kMaxReelDepth = 8;
    bool addressed_ = false;
    double atBeat_ = 0.0, atTempo_ = 120.0;
    float frameDelta_ = 1.0f / 30.0f;
    bool exact_ = false;
    std::string poolPrefix_;

    static constexpr double kSlowestSource = 1.0;

    static void keepFrame(DeckState& d, double pts) {
        if (const double step = pts - d.shownPts;
            d.shownPts >= 0.0 && step > 1.0e-4 && step < kSlowestSource)
            d.sourceStep = d.sourceStep > 0.0 ? std::min(d.sourceStep, step) : step;
        d.shownPts = pts;
    }

    static void settle(VideoLayer& layer, double seconds, DeckState& d) {
        constexpr double kAhead = 0.001, kNear = 0.002;
        constexpr int kFresh = 2, kRepeat = 40, kTries = 2000, kNap = 1;
        double best = -2.0;
        int steady = 0;
        for (int i = 0; i < kTries; ++i) {
            const auto f = layer.latestFrame();
            const double pts = f != nullptr ? f->pts : -1.0;
            if (pts >= 0.0 && pts <= seconds + kAhead) {
                if (seconds - pts <= kNear
                    || (d.sourceStep > 0.0 && seconds - pts < d.sourceStep))
                    return keepFrame(d, pts);
                const bool fresh = std::abs(pts - d.shownPts) > 1.0e-9;
                if (std::abs(pts - best) < 1.0e-9) {
                    if (++steady >= (fresh ? kFresh : kRepeat)) return keepFrame(d, pts);
                } else {
                    best = pts;
                    steady = 0;
                }
            }
            juce::Thread::sleep(kNap);
        }
        if (std::getenv("HUMUS_BOUNCE_DEBUG") != nullptr) {
            const auto f = layer.latestFrame();
            std::fprintf(stderr, "[settle] gave up: want %.4f latest %s pts %.4f best %.4f shown %.4f step %.4f\n",
                         seconds, f != nullptr ? "frame" : "none", f != nullptr ? f->pts : -1.0,
                         best, d.shownPts, d.sourceStep);
        }
        if (best >= 0.0) keepFrame(d, best);
    }

    int stageTrackClip(const std::string& node, const VideoTimelineSource& track,
                       const VideoTimelineSource::Cue& cue, bool wantFrame, visual::Plan& p,
                       std::set<std::string>& live, int depth = 0) {
        const auto tape = track.cueFile(cue.clip);
        if (auto* reel = dynamic_cast<VideoTimelineSource*>(host_.liveOrganism(tape));
            reel != nullptr && depth < kMaxReelDepth) {
            const double tempo = addressed_ ? atTempo_ : host_.model().clock.tempo;
            const double beat = cue.seconds * std::max(1.0, tempo) / kSecondsPerMinute;
            auto inner = reel->cueAt(beat, tempo);
            if (inner.clip < 0) return -1;
            inner.rolling = cue.rolling;
            inner.level *= cue.level;
            return stageTrackClip(node + "/c" + std::to_string(cue.clip), *reel, inner,
                                  wantFrame, p, live, depth + 1);
        }
        const auto key = node + "/c" + std::to_string(cue.clip);
        live.insert(key);
        auto& d = decks_[key];
        auto layer = openDeck(d, key, juce::String(tape));
        if (layer == nullptr) return -1;
        layer->chase(cue.seconds, exact_ ? 0.0 : cue.rolling ? cue.rate : 0.0);
        if (!wantFrame) return -1;
        if (exact_) settle(*layer, cue.seconds, d);
        visual::Step ds;
        ds.kind = visual::Step::Deck;
        ds.node = key;
        auto f = layer->latestFrame();
        if (f != nullptr) d.hadFrame = true;
        ds.frame = std::move(f);
        ds.active = d.hadFrame;
        const int idx = (int) p.steps.size();
        p.steps.push_back(std::move(ds));
        return idx;
    }

    bool trackShowsInput(const std::string& node, const VideoTimelineSource& track) {
        if (addressed_) return false;
        const int monitor = (int) paramOr(node, "Monitor", 1.0f);
        const bool armed = paramOr(node, "Record", 0.0f) >= 0.5f;
        return monitor == 0 || (monitor == 1 && (armed || !host_.isPlaying()));
    }

    void buildTrackStep(const std::string& node, const VideoTimelineSource& track,
                        visual::Step& s, visual::Plan& p, std::set<std::string>& live,
                        int input) {
        s.mixA = input;
        s.mixB = -1;
        s.mixFade = 0.0f;
        const auto cue = addressed_ ? track.cueAt(atBeat_, atTempo_) : track.cue();
        if (cue.clip >= 0) {
            const int deck = stageTrackClip(node, track, cue, input < 0, p, live);
            if (input < 0) {
                s.mixB = deck;
                s.mixFade = deck >= 0 ? cue.level : 0.0f;
            }
        }
        const auto next = addressed_ ? track.upcomingAt(atBeat_, atTempo_) : track.upcoming();
        if (next.clip >= 0 && next.clip != cue.clip) stageTrackClip(node, track, next, false, p, live);
    }

    void buildDeckStep(const std::string& node, visual::Step& s) {
        auto& d = decks_[node];
        if (auto* cam = dynamic_cast<CamPreviewSource*>(host_.liveOrganism(node)))
            return buildCameraStep(*cam, d, s);
        if (openDeck(d, node, paramText(node, "File")) == nullptr) return;
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
    struct RetiredDeck {
        std::shared_ptr<VideoLayer> layer;
        double at = 0.0;
    };
    static constexpr double kRetireGraceMs = 2000.0;
    std::map<std::string, DeckState> decks_;
    std::vector<RetiredDeck> retired_;
    std::set<std::string> tapped_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VisualPlanBuilder)
};

}
