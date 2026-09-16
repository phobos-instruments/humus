// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
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
#include <juce_events/juce_events.h>

#include "core/app/AppPaths.h"
#include "core/assistant/AssistantProtocol.h"
#include "core/assistant/AutomixPlan.h"
#include "core/params/ParamSchema.h"
#include "core/graph/PodModel.h"
#include "core/packs/PackRegistry.h"
#include "core/analysis/SpectrumBands.h"
#include "core/packs/Roles.h"
#include "gui/assistant/AiClient.h"
#include "gui/host/AssistantHost.h"
#include "hum/Registry.h"
#include "gui/common/Localisation.h"

namespace hum {

class AssistantEngine {
public:
    explicit AssistantEngine(AssistantHost& host);
    ~AssistantEngine();

    std::function<void(const juce::String& role, const juce::String& text)> onLine;
    std::function<void(bool)> onBusy;
    std::function<void()> onPatchEdited;

    bool busy() const { return busy_; }
    void resetConversation() { messages_ = juce::Array<juce::var>(); }

    void runToolForTest(const juce::String& name, std::function<void(juce::String)> done);

    void send(const juce::String& userText);

private:
    static constexpr int kMaxRounds = 12;

    void line(const juce::String& role, const juce::String& text);
    void setBusy(bool b);
    void finishTurn();

    void continueLoop();

    void runToolChain(std::shared_ptr<std::vector<ToolCall>> calls, size_t i, std::shared_ptr<std::vector<std::pair<juce::String, juce::String>>> results);

    void runToolAsync(const ToolCall& c, std::function<void(juce::String)> done);

    juce::String runTool(const ToolCall& c);

    static juce::String unknownNode(const juce::var& n);

    static std::string resolveClass(const std::string& cls);

    static juce::String unknownClass(const std::string& cls);

    juce::String patchJson() const;

    static constexpr const char* kNotPlaying =
        "error: the transport is stopped - start playback (transport tool) first";

    class Ticker : public juce::Timer {
    public:
        void start(int intervalMs, int ticks, std::function<void()> onTick,
                   std::function<void()> onDone) {
            stopTimer();
            left_ = ticks;
            onTick_ = std::move(onTick);
            onDone_ = std::move(onDone);
            startTimer(intervalMs);
        }
        void timerCallback() override {
            if (onTick_) onTick_();
            if (--left_ > 0) return;
            stopTimer();
            auto done = std::move(onDone_);
            onDone_ = nullptr;
            if (done) done();
        }

    private:
        int left_ = 0;
        std::function<void()> onTick_;
        std::function<void()> onDone_;
    };

    static constexpr int kPeakTicks = 24;
    static constexpr int kPeakTickMs = 25;

    void measurePeaksAsync(std::function<void(const std::map<std::string, float>&)> done);

    juce::String levelsJson(const std::map<std::string, float>& peaksIn);

    juce::String spectrumJsonFromCapture(double sr);

    struct AutomixStage {
        juce::Array<juce::var> moved;
        juce::StringArray skipped;
        std::string anchor;
        std::string master;
        juce::String error;
    };

    static bool fileTextAllowed(const std::string& text);

    static juce::String levelParamOf(const std::string& cls);

    static void paramRange(const std::string& cls, const juce::String& param, double& mn, double& mx);

    void applyMove(const std::string& node, const juce::String& param, double from, double to, juce::Array<juce::var>& moved);

    AutomixStage automixStage(const std::map<std::string, float>& peaksIn);

    juce::String automixFinish(AutomixStage st, const std::map<std::string, float>& peaks);

    juce::String classesJson() const;

    juce::String describeJson(const std::string& nameOrClass) const;

    AssistantHost& host_;
    Ticker ticker_;
    juce::var messages_;
    int rounds_ = 0;
    bool busy_ = false;
    bool edited_ = false;
    bool nudged_ = false;
    std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);
};

}
