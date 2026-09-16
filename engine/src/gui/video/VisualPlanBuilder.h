// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
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

#include "core/app/AppPaths.h"
#include "core/video/IsfParse.h"
#include "core/video/SsfShim.h"
#include "core/video/VideoGraph.h"
#include "gui/video/SceneAssemble.h"
#include "core/video/VisualUniforms.h"
#include "gui/host/VideoHost.h"
#include "hum/Organism.h"
#include "io/PatchDocument.h"
#include "gui/video/VideoDeckPool.h"
#include "gui/video/VideoLayer.h"
#include "gui/video/VideoTakeSink.h"
#include "gui/video/VisualGlCanvas.h"
#include "gui/video/VisualPlan.h"
#include "hum/caps/Audio.h"
#include "hum/caps/Video.h"

namespace hum {

inline bool isVideoOutputNode(VideoHost& host, const std::string& node) {
    auto* org = host.liveOrganism(node);
    if (auto* v = dynamic_cast<VideoNode*>(org))
        return v->numVideoOutputs() == 0 && v->numVideoInputs() > 0
               && dynamic_cast<VisualSource*>(org) == nullptr
               && dynamic_cast<VideoFrameSink*>(org) == nullptr;
    return false;
}

class VisualPlanBuilder {
public:
    VisualPlanBuilder(VideoHost& host, std::string node, bool isOutput)
        : host_(host), name_(std::move(node)), isOutput_(isOutput) {}

    explicit VisualPlanBuilder(VideoHost& host) : host_(host), isOutput_(false) {}

    ~VisualPlanBuilder();

    struct Want {
        std::string node;
        bool isOutput = true;
        int w = 0, h = 0;
        bool everyOther = false;
        std::shared_ptr<VideoTakeSink> sink;
    };

    void evaluateAt(double beat, double tempo, float frameDelta = 1.0f / 30.0f);
    void evaluateLive() { addressed_ = false; }
    void setExactFrames(bool on);
    const std::string& poolPrefix() const { return poolPrefix_; }

    visual::Plan buildAt(double beat, double tempo, float frameDelta = 1.0f / 30.0f);

    visual::Plan build();

    visual::Plan buildAll(const std::vector<Want>& wants);

    const std::string& root() const { return root_; }

    juce::String parseError() const;

    static int stepFor(const std::map<std::string, int>& stepOf, const std::string& src, int outlet);

    int sourceStep(const std::string& node, int inlet, const std::map<std::string, int>& stepOf, visual::Plan* stack = nullptr);

    float paramOr(const std::string& node, const char* param, float def);

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

    juce::String paramText(const std::string& node, const juce::String& param) const;

    juce::File resolvePath(const juce::String& path) const;

    void adoptScene(SceneState& st, AssembledScene a);

    void buildSceneStep(const std::string& node, visual::Step& s, float dt, bool pollMtime);

    void buildCameraStep(CamPreviewSource& cam, DeckState& d, visual::Step& s);

    void buildFxStep(const std::string& node, visual::Step& s);

    std::shared_ptr<VideoLayer> openDeck(DeckState& d, const std::string& key, const juce::String& path);

    bool padOutletWired(const std::string& node, int slot) const;

    void buildClipsStep(const std::string& node, VideoPadSource& clips, visual::Step& s, visual::Plan& p, std::set<std::string>& live, std::map<std::string, int>& stepOf);

    static constexpr int kMaxReelDepth = 8;
    bool addressed_ = false;
    double atBeat_ = 0.0, atTempo_ = 120.0;
    float frameDelta_ = 1.0f / 30.0f;
    bool exact_ = false;
    std::string poolPrefix_;

    static constexpr double kSlowestSource = 1.0;

    static void keepFrame(DeckState& d, double pts);

    static void settle(VideoLayer& layer, double seconds, DeckState& d);

    int stageTrackClip(const std::string& node, const VideoTimelineSource& track, const VideoTimelineSource::Cue& cue, bool wantFrame, visual::Plan& p, std::set<std::string>& live, int depth = 0);

    bool trackShowsInput(const std::string& node, const VideoTimelineSource& track);

    void buildTrackStep(const std::string& node, const VideoTimelineSource& track, visual::Step& s, visual::Plan& p, std::set<std::string>& live, int input);

    void buildDeckStep(const std::string& node, visual::Step& s);

    VideoHost& host_;
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
