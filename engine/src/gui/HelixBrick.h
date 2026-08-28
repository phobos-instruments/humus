#pragma once
#include <array>
#include <cmath>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/LookAndFeel.h"
#include "gui/PolledBrick.h"
#include "hum/Capabilities.h"

namespace hum {

class HelixStrandView : public PolledBrick, public juce::FileDragAndDropTarget {
public:
    bool isInterestedInFileDrag(const juce::StringArray& files) override {
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        return files.size() == 1 && fm.findFormatForFileExtension(juce::File(files[0]).getFileExtension()) != nullptr;
    }
    void filesDropped(const juce::StringArray& files, int x, int) override {
        host_.setParamText(name_, "Loop" + std::to_string(strandAtX(x) + 1),
                           juce::File(files[0]).getFullPathName().toStdString());
        if (auto* sa = live<SessionAudio>()) sa->loadSessionAudio();
        dropStrand_ = -1;
        repaint();
    }
    void fileDragEnter(const juce::StringArray&, int x, int) override { dropStrand_ = strandAtX(x); repaint(); }
    void fileDragMove(const juce::StringArray&, int x, int) override {
        if (const int s = strandAtX(x); s != dropStrand_) { dropStrand_ = s; repaint(); }
    }
    void fileDragExit(const juce::StringArray&) override { dropStrand_ = -1; repaint(); }
    HelixStrandView(EngineHost& host, std::string organism)
        : PolledBrick(host, std::move(organism), 2) {
        lastState_.fill(-1);
        lastLayers_.fill(-1);
        lastPhase_.fill(-1);
        lastPending_.fill(false);
    }

    int preferredContentWidth() const override { return 584; }
    int preferredContentHeight(int) const override { return 52; }

    void paint(juce::Graphics& g) override {
        auto* st = live<StrandStatus>();
        const int n = st ? st->strandCount() : 4;
        if (n <= 0) return;
        const float colW = (float) getWidth() / (float) n;
        const float r = std::min(colW * 0.5f, (float) getHeight() * 0.5f) - 8.0f;
        const juce::Colour red(0xffe23b3b), amber(0xffe0a03c);
        bool anySolo = false;
        for (int i = 0; i < n; ++i)
            anySolo = anySolo || host_.liveParamValue(name_, "Solo" + std::to_string(i + 1)) >= 0.5;

        for (int i = 0; i < n; ++i) {
            const int state = st ? st->strandState(i) : 0;
            const float phase = st ? st->strandPhase(i) : -1.0f;
            const int layers = st ? st->strandLayers(i) : 0;
            const bool pending = st && st->strandPending(i);
            const std::string k = std::to_string(i + 1);
            const bool audible = host_.liveParamValue(name_, "Mute" + k) < 0.5
                                 && (!anySolo || host_.liveParamValue(name_, "Solo" + k) >= 0.5);
            const float cx = colW * ((float) i + 0.5f);
            const float cy = (float) getHeight() * 0.5f;
            const juce::Rectangle<float> ring(cx - r, cy - r, r * 2.0f, r * 2.0f);

            juce::Colour c = Palette::border;
            if (state == StrandStatus::kRecord) c = red;
            else if (state == StrandStatus::kDub) c = amber;
            else if (state == StrandStatus::kPlay) c = Palette::accent;
            else if (state == StrandStatus::kStopped) c = Palette::textDim;
            if (!audible) c = c.withAlpha(0.3f);
            if (pending && (blink_ & 4) != 0) c = c.withAlpha(0.35f);

            if (state == StrandStatus::kRecord || state == StrandStatus::kDub) {
                g.setColour(c.withAlpha(0.25f));
                g.fillEllipse(ring);
            }
            g.setColour(c);
            g.drawEllipse(ring, state == StrandStatus::kEmpty ? 1.0f : 2.0f);

            if (i == dropStrand_) {
                g.setColour(Palette::accent);
                g.drawEllipse(ring.expanded(3.0f), 2.0f);
            }

            if (phase >= 0.0f) {
                juce::Path arc;
                arc.addArc(ring.getX() + 3.0f, ring.getY() + 3.0f,
                           ring.getWidth() - 6.0f, ring.getHeight() - 6.0f,
                           0.0f, phase * juce::MathConstants<float>::twoPi, true);
                g.setColour(c);
                g.strokePath(arc, juce::PathStrokeType(3.0f));
            }
            if (layers > 0) {
                g.setColour(state == StrandStatus::kStopped ? Palette::textDim : Palette::text);
                g.setFont(juce::FontOptions(11.0f));
                g.drawText(juce::String(layers), ring.toNearestInt(),
                           juce::Justification::centred, false);
            }
        }
    }

private:
    void poll() override {
        auto* st = live<StrandStatus>();
        bool moved = false;
        bool anyPending = false;
        for (int i = 0; i < (int) lastState_.size(); ++i) {
            const int state = st ? st->strandState(i) : 0;
            const int layers = st ? st->strandLayers(i) : 0;
            const std::string k = std::to_string(i + 1);
            const int ms = (host_.liveParamValue(name_, "Mute" + k) >= 0.5 ? 1 : 0)
                           + (host_.liveParamValue(name_, "Solo" + k) >= 0.5 ? 2 : 0);
            const bool pending = st && st->strandPending(i);
            const float ph = st ? st->strandPhase(i) : -1.0f;
            const int phq = ph < 0.0f ? -1 : (int) (ph * 96.0f);
            const auto j = (size_t) i;
            moved = moved || state != lastState_[j] || layers != lastLayers_[j]
                    || pending != lastPending_[j] || phq != lastPhase_[j]
                    || ms != lastMuteSolo_[j];
            lastState_[j] = state;
            lastLayers_[j] = layers;
            lastMuteSolo_[j] = ms;
            lastPending_[j] = pending;
            lastPhase_[j] = phq;
            anyPending = anyPending || pending;
        }
        if (anyPending) ++blink_;
        if (moved || anyPending) repaint();
    }

    std::array<int, 4> lastState_ {}, lastLayers_ {}, lastPhase_ {}, lastMuteSolo_ {};
    int dropStrand_ = -1;
    int strandAtX(int x) const {
        auto* st = live<StrandStatus>();
        const int n = st ? st->strandCount() : 4;
        return juce::jlimit(0, n - 1, (int) ((float) x / ((float) getWidth() / (float) n)));
    }
    std::array<bool, 4> lastPending_ {};
    int blink_ = 0;
};

}
