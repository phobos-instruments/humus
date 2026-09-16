// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/video/VisualPlanBuilder.h"
#include "hum/caps/Audio.h"
#include "hum/caps/Video.h"

#include <cstdint>
#include <cstdlib>

namespace hum {

VisualPlanBuilder::~VisualPlanBuilder() {
    for (const auto& n : tapped_)
        if (auto* vs = dynamic_cast<VisualSource*>(host_.liveOrganism(n)))
            vs->setVisualTapEnabled(false);
}

void VisualPlanBuilder::evaluateAt(double beat, double tempo, float frameDelta) {
    atBeat_ = beat;
    atTempo_ = tempo > 0.0 ? tempo : 120.0;
    frameDelta_ = frameDelta > 0.0f ? frameDelta : 1.0f / 30.0f;
    addressed_ = true;
}

void VisualPlanBuilder::setExactFrames(bool on) {
    exact_ = on;
    poolPrefix_ = on ? "offline" + std::to_string((std::uintptr_t) this) + "/" : std::string();
}

visual::Plan VisualPlanBuilder::buildAt(double beat, double tempo, float frameDelta) {
    evaluateAt(beat, tempo, frameDelta);
    auto p = build();
    evaluateLive();
    return p;
}

visual::Plan VisualPlanBuilder::build() {
    auto p = buildAll({{name_, isOutput_, 0, 0, false}});
    p.root = p.taps[0].step;
    p.masterFade = p.taps[0].fade;
    if (host_.bypassed(name_)) p.noSignal = true;
    p.taps.clear();
    return p;
}

visual::Plan VisualPlanBuilder::buildAll(const std::vector<Want>& wants) {
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

juce::String VisualPlanBuilder::parseError() const {
    for (const auto& [node, st] : scenes_)
        if (st.parseError.isNotEmpty())
            return juce::String(node) + ": " + st.parseError;
    return {};
}

int VisualPlanBuilder::stepFor(const std::map<std::string, int>& stepOf, const std::string& src, int outlet) {
    const auto it = stepOf.find(outlet > 0 ? src + "#" + std::to_string(outlet) : src);
    return it != stepOf.end() ? it->second : -1;
}

int VisualPlanBuilder::sourceStep(const std::string& node, int inlet, const std::map<std::string, int>& stepOf, visual::Plan* stack) {
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

float VisualPlanBuilder::paramOr(const std::string& node, const char* param, float def) {
    if (auto* c = host_.liveOrganism(node))
        if (c->params.byName(param) != nullptr)
            return (float) host_.liveParamValue(node, param);
    return def;
}

juce::String VisualPlanBuilder::paramText(const std::string& node, const juce::String& param) const {
    if (const auto* cm = host_.model().byName(node))
        for (const auto& p : cm->properties)
            if (p.name == param.toStdString()) return juce::String(p.text);
    return {};
}

juce::File VisualPlanBuilder::resolvePath(const juce::String& path) const {
    return VideoDeckPool::resolveTape(host_.documentPath(), path);
}

void VisualPlanBuilder::adoptScene(SceneState& st, AssembledScene a) {
    st.parseError = std::move(a.parseError);
    st.isfInputs = std::move(a.isfInputs);
    st.ssfControls = std::move(a.ssfControls);
    st.ssf = a.ssf;
    st.shk = a.shk;
    st.spec = std::move(a.spec);
}

void VisualPlanBuilder::keepFrame(DeckState& d, double pts) {
    if (const double step = pts - d.shownPts;
        d.shownPts >= 0.0 && step > 1.0e-4 && step < kSlowestSource)
        d.sourceStep = d.sourceStep > 0.0 ? std::min(d.sourceStep, step) : step;
    d.shownPts = pts;
}

void VisualPlanBuilder::settle(VideoLayer& layer, double seconds, DeckState& d) {
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

}
