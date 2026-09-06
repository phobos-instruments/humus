#pragma once
#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/LookAndFeel.h"
#include "gui/PolledBrick.h"
#include "hum/dsp/SoundFileBuffer.h"
#include "hum/dsp/WaveTable.h"
#include "hum/dsp/WaveWarp.h"
#include "gui/Localisation.h"

namespace hum {

class WaveDrawBrick : public PolledBrick {
public:
    WaveDrawBrick(EngineHost& host, std::string organism, std::string param)
        : PolledBrick(host, std::move(organism), 4), pn_(std::move(param)) {
        pull(host_.liveParamText(name_, pn_));
    }

    void reloadValues() override { pull(host_.liveParamText(name_, pn_)); }
    void refreshAutomatedValues() override {}
    int preferredContentWidth() const override { return 584; }
    int preferredContentHeight(int) const override { return 150; }

    void paint(juce::Graphics& g) override {
        const auto area = waveArea().toFloat();
        g.setColour(Palette::background);
        g.fillRoundedRectangle(area, 4.0f);
        const float cy = area.getCentreY();
        g.setColour(Palette::border.withAlpha(0.35f));
        for (int q = 1; q < 4; ++q)
            g.drawVerticalLine((int) (area.getX() + area.getWidth() * (float) q / 4.0f),
                               area.getY() + 2, area.getBottom() - 2);
        g.drawHorizontalLine((int) cy, area.getX() + 2, area.getRight() - 2);

        juce::Path p;
        const int steps = juce::jmax(64, (int) area.getWidth());
        p.startNewSubPath(area.getX(), cy);
        for (int i = 0; i <= steps; ++i) {
            const double ph = (double) i / steps;
            const float v = valueAt(ph);
            p.lineTo(area.getX() + (float) ph * area.getWidth(),
                     cy - v * area.getHeight() * 0.46f);
        }
        p.lineTo(area.getRight(), cy);
        p.closeSubPath();
        g.setColour(Palette::accent.withAlpha(0.3f));
        g.fillPath(p);
        g.setColour(Palette::accent);
        g.strokePath(p, juce::PathStrokeType(2.0f));

        const int wm = (int) host_.liveParamValue(name_, "WarpMode");
        const float wa = (float) host_.liveParamValue(name_, "Warp");
        if (wm > 0 && wa > 0.0f) {
            static thread_local float disp[kWaveTableLen];
            int df0, df1; float dfr; blend(df0, df1, dfr);
            for (int k = 0; k < kWaveTableLen; ++k)
                disp[k] = frames_[df0][k] + dfr * (frames_[df1][k] - frames_[df0][k]);
            juce::Path wp;
            for (int i = 0; i <= steps; ++i) {
                const double ph = (double) i / steps;
                const float v = warpRead(disp, kWaveTableLen, ph, wm, wa);
                const float y = cy - juce::jlimit(-1.0f, 1.0f, v) * area.getHeight() * 0.46f;
                if (i == 0) wp.startNewSubPath(area.getX(), y);
                else wp.lineTo(area.getX() + (float) ph * area.getWidth(), y);
            }
            g.setColour(Palette::text.withAlpha(0.75f));
            g.strokePath(wp, juce::PathStrokeType(1.5f));
        }

        g.setColour(Palette::border);
        g.drawRoundedRectangle(area, 4.0f, 1.0f);

        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(11.0f));
        g.drawText(frameCount_ > 1
                       ? juce::String("frame ") + juce::String(curFrame() + 1) + " / "
                             + juce::String(frameCount_)
                       : juce::String("1 frame  -  Seed a file for a wavetable"),
                   area.toNearestInt().reduced(8, 6), juce::Justification::topRight, false);

        paintStrip(g);

        for (int i = 0; i < kWaveTablePresets; ++i) {
            const auto r = tileRect(i).toFloat();
            const bool on = i == activePreset_;
            g.setColour(on ? Palette::accent : Palette::panelLight);
            g.fillRoundedRectangle(r, 4.0f);
            float mini[32];
            waveTablePreset(i, mini, 32);
            juce::Path mp;
            const auto inner = r.reduced(6.0f, 5.0f);
            for (int k = 0; k <= 32; ++k) {
                const juce::Point<float> pt(
                    inner.getX() + (float) k / 32.0f * inner.getWidth(),
                    inner.getCentreY() - mini[k & 31] * inner.getHeight() * 0.5f);
                if (k == 0) mp.startNewSubPath(pt);
                else mp.lineTo(pt);
            }
            g.setColour(on ? Palette::background : Palette::textDim);
            g.strokePath(mp, juce::PathStrokeType(1.4f));
        }
        for (int t = 0; t < 2; ++t) {
            const auto r = tileRect(kWaveTablePresets + t).toFloat();
            g.setColour(Palette::panelLight);
            g.fillRoundedRectangle(r, 4.0f);
            g.setColour(Palette::text);
            g.setFont(juce::FontOptions(11.0f));
            g.drawText(t == 0 ? tr("wave-draw.random", "Random") : tr("wave-draw.seed-file", "Seed file..."), r, juce::Justification::centred);
        }
        for (int t = 2; t < 4; ++t) {
            const auto r = tileRect(kWaveTablePresets + t).toFloat();
            g.setColour(Palette::panelLight);
            g.fillRoundedRectangle(r, 4.0f);
            g.setColour(frameCount_ >= (t == 2 ? kMaxFrames : 2) ? Palette::textDim : Palette::text);
            g.setFont(juce::FontOptions(15.0f));
            g.drawText(t == 2 ? tr("wave-draw.frame", "+ frame") : tr("wave-draw.frame-2", "- frame"), r, juce::Justification::centred);
        }
    }

    void mouseDown(const juce::MouseEvent& e) override {
        for (int i = 0; i < kWaveTablePresets; ++i)
            if (tileRect(i).contains(e.getPosition())) {
                waveTablePreset(i, frames_[curFrame()], kWaveTableLen);
                activePreset_ = i;
                push();
                repaint();
                return;
            }
        if (tileRect(kWaveTablePresets).contains(e.getPosition())) {
            waveTableRandom((uint32_t) juce::Random::getSystemRandom().nextInt(),
                            frames_[curFrame()], kWaveTableLen);
            activePreset_ = -1;
            push();
            repaint();
            return;
        }
        if (tileRect(kWaveTablePresets + 1).contains(e.getPosition())) {
            chooseSeed();
            return;
        }
        if (tileRect(kWaveTablePresets + 2).contains(e.getPosition())) { addFrame(); return; }
        if (tileRect(kWaveTablePresets + 3).contains(e.getPosition())) { removeFrame(); return; }
        if (frameCount_ > 1 && stripRect().contains(e.getPosition())) {
            const int n = frameCount_;
            const int f = juce::jlimit(0, n - 1,
                (e.x - stripRect().getX()) * n / std::max(1, stripRect().getWidth()));
            host_.setParam(name_, "Position", n > 1 ? (double) f / (n - 1) : 0.0);
            return;
        }
        if (waveArea().contains(e.getPosition())) {
            drawing_ = true;
            lastIdx_ = -1;
            paintSample(e);
        }
    }
    void mouseDrag(const juce::MouseEvent& e) override {
        if (drawing_) paintSample(e);
    }
    void mouseUp(const juce::MouseEvent&) override {
        if (!drawing_) return;
        drawing_ = false;
        activePreset_ = -1;
        push();
        repaint();
    }
    void mouseMove(const juce::MouseEvent& e) override {
        setMouseCursor(waveArea().contains(e.getPosition())
                           ? juce::MouseCursor::CrosshairCursor
                           : juce::MouseCursor::NormalCursor);
    }

private:
    static constexpr int kTileH = 26, kTileGap = 4, kTiles = kWaveTablePresets + 4;

    static constexpr int kStripH = 20, kStripGap = 4;
    juce::Rectangle<int> stripRect() const {
        auto r = getLocalBounds();
        r.removeFromBottom(kTileH + kTileGap);
        return r.removeFromBottom(kStripH);
    }
    juce::Rectangle<int> waveArea() const {
        auto r = getLocalBounds();
        r.removeFromBottom(kTileH + kTileGap + kStripH + kStripGap);
        return r;
    }
    juce::Rectangle<int> frameSlot(int f) const {
        const auto s = stripRect();
        const int n = std::max(1, frameCount_);
        const int w = s.getWidth() / n;
        return {s.getX() + f * w, s.getY(), w - 1, s.getHeight()};
    }
    juce::Rectangle<int> tileRect(int i) const {
        const int w = (getWidth() - (kTiles - 1) * kTileGap) / kTiles;
        return {i * (w + kTileGap), getHeight() - kTileH, w, kTileH};
    }

    int curFrame() const { return juce::jlimit(0, frameCount_ - 1,
                                                (int) std::lround(pos_ * (frameCount_ - 1))); }
    void blend(int& f0, int& f1, float& fr) const {
        const double fp = pos_ * (frameCount_ - 1);
        f0 = juce::jlimit(0, frameCount_ - 1, (int) fp);
        f1 = juce::jmin(f0 + 1, frameCount_ - 1);
        fr = (float) (fp - f0);
    }
    float valueAt(double phase) const {
        int f0, f1; float fr; blend(f0, f1, fr);
        const double f = phase * (kWaveTableLen - 1);
        const int i = juce::jlimit(0, kWaveTableLen - 2, (int) f);
        const float t = (float) (f - i);
        auto samp = [&](int fi) { return frames_[fi][i] * (1.0f - t) + frames_[fi][i + 1] * t; };
        return samp(f0) + fr * (samp(f1) - samp(f0));
    }

    void paintSample(const juce::MouseEvent& e) {
        const auto area = waveArea();
        const int idx = juce::jlimit(
            0, kWaveTableLen - 1,
            (int) std::lround((double) (e.x - area.getX()) / area.getWidth()
                              * (kWaveTableLen - 1)));
        const float v = (float) juce::jlimit(
            -1.0, 1.0,
            ((double) area.getCentreY() - e.y) / (area.getHeight() * 0.46));
        float* frame = frames_[curFrame()];
        if (lastIdx_ < 0) {
            frame[idx] = v;
        } else {
            const int a = juce::jmin(lastIdx_, idx), b = juce::jmax(lastIdx_, idx);
            for (int i = a; i <= b; ++i) {
                const float t = b == a ? 1.0f : (float) (i - a) / (float) (b - a);
                const float from = idx >= lastIdx_ ? lastVal_ : v;
                const float to = idx >= lastIdx_ ? v : lastVal_;
                frame[i] = from + (to - from) * t;
            }
        }
        lastIdx_ = idx;
        lastVal_ = v;
        repaint();
    }

    void addFrame() {
        if (frameCount_ >= kMaxFrames) return;
        const int at = curFrame();
        for (int f = frameCount_; f > at + 1; --f)
            std::copy(frames_[f - 1], frames_[f - 1] + kWaveTableLen, frames_[f]);
        std::copy(frames_[at], frames_[at] + kWaveTableLen, frames_[at + 1]);
        ++frameCount_;
        push();
        host_.setParam(name_, "Position", frameCount_ > 1 ? (double) (at + 1) / (frameCount_ - 1) : 0.0);
        repaint();
    }
    void removeFrame() {
        if (frameCount_ <= 1) return;
        const int at = curFrame();
        for (int f = at; f + 1 < frameCount_; ++f)
            std::copy(frames_[f + 1], frames_[f + 1] + kWaveTableLen, frames_[f]);
        --frameCount_;
        push();
        const int nc = juce::jmin(at, frameCount_ - 1);
        host_.setParam(name_, "Position", frameCount_ > 1 ? (double) nc / (frameCount_ - 1) : 0.0);
        repaint();
    }

    void paintStrip(juce::Graphics& g) {
        const auto s = stripRect();
        g.setColour(Palette::background.darker(0.1f));
        g.fillRoundedRectangle(s.toFloat(), 3.0f);
        if (frameCount_ <= 1) {
            g.setColour(Palette::textDim);
            g.setFont(juce::FontOptions(10.0f));
            g.drawText(tr("wave-draw.one-frame", "one frame"), s, juce::Justification::centred);
            return;
        }
        const int cur = curFrame();
        for (int f = 0; f < frameCount_; ++f) {
            const auto slot = frameSlot(f).toFloat().reduced(1.0f, 2.0f);
            if (f == cur) {
                g.setColour(Palette::accent.withAlpha(0.22f));
                g.fillRect(slot);
            }
            juce::Path mp;
            const int steps = std::max(8, (int) slot.getWidth());
            const float cy = slot.getCentreY();
            for (int k = 0; k <= steps; ++k) {
                const double ph = (double) k / steps;
                const int idx = juce::jlimit(0, kWaveTableLen - 1, (int) (ph * (kWaveTableLen - 1)));
                const float y = cy - frames_[f][idx] * slot.getHeight() * 0.42f;
                const juce::Point<float> pt(slot.getX() + (float) ph * slot.getWidth(), y);
                if (k == 0) mp.startNewSubPath(pt); else mp.lineTo(pt);
            }
            g.setColour(f == cur ? Palette::accent : Palette::text.withAlpha(0.4f));
            g.strokePath(mp, juce::PathStrokeType(1.0f));
        }
    }

    void chooseSeed() {
        chooser_ = std::make_unique<juce::FileChooser>(
            "Seed a wave from any file", juce::File(), "*");
        const auto flags = juce::FileBrowserComponent::openMode
                         | juce::FileBrowserComponent::canSelectFiles;
        chooser_->launchAsync(flags, [this](const juce::FileChooser& fc) {
            const auto f = fc.getResult();
            if (f == juce::File()) return;
            seedFromFile(f);
        });
    }

    void seedFromFile(const juce::File& f) {
        if (const auto img = juce::ImageFileFormat::loadFrom(f); img.isValid()) {
            const int w = juce::jmin(img.getWidth(), 4096);
            std::vector<float> row((size_t) w);
            const int y = img.getHeight() / 2;
            for (int x = 0; x < w; ++x)
                row[(size_t) x] = img.getPixelAt(x, y).getBrightness() * 2.0f - 1.0f;
            waveTableFromCycle(row.data(), w, frames_[0], kWaveTableLen);
            frameCount_ = 1;
        } else if (juce::AudioBuffer<float> buf; loadAudioSeed(f, buf)) {
        } else if (juce::FileInputStream in(f); in.openedOk() && in.getTotalLength() > 0) {
            const auto size = in.getTotalLength();
            const int take = (int) juce::jmin(size, (juce::int64) 65536);
            in.setPosition((size - take) / 2);
            std::vector<uint8_t> bytes((size_t) take);
            in.read(bytes.data(), take);
            waveTableFromBytes(bytes.data(), take, frames_[0], kWaveTableLen);
            frameCount_ = 1;
        } else {
            return;
        }
        activePreset_ = -1;
        push();
        repaint();
    }

    bool loadAudioSeed(const juce::File& f, juce::AudioBuffer<float>& buf) {
        double fileSr = 0.0;
        if (!loadSoundFile(f.getFullPathName().toStdString(), buf, fileSr)
            || buf.getNumSamples() < 256)
            return false;
        const int n = buf.getNumSamples();
        const int take = juce::jmin(n, (int) (2.0 * fileSr));
        const int start = (n - take) / 2;
        std::vector<float> mono((size_t) take, 0.0f);
        for (int c = 0; c < buf.getNumChannels(); ++c) {
            const float* src = buf.getReadPointer(c, start);
            for (int i = 0; i < take; ++i)
                mono[(size_t) i] += src[i] / (float) buf.getNumChannels();
        }
        frameCount_ = waveTableFramesFromSignal(mono.data(), take, fileSr,
                                                &frames_[0][0], kWaveTableLen, kMaxFrames);
        waveTableAlignFrames(&frames_[0][0], frameCount_, kWaveTableLen);
        if (frameCount_ <= 0) {
            waveTableFromCycle(mono.data(), juce::jmin(take, 4096), frames_[0], kWaveTableLen);
            frameCount_ = 1;
        }
        return true;
    }

    void pull(const std::string& text) {
        cachedText_ = text;
        const int n = decodeWaveFrames(text.c_str(), &frames_[0][0], kWaveTableLen, kMaxFrames);
        frameCount_ = n > 0 ? n : 1;
        waveTableAlignFrames(&frames_[0][0], frameCount_, kWaveTableLen);
        if (n <= 0) waveTablePreset(0, frames_[0], kWaveTableLen);
        activePreset_ = -1;
        if (frameCount_ == 1)
            for (int i = 0; i < kWaveTablePresets; ++i) {
                float t[kWaveTableLen];
                waveTablePreset(i, t, kWaveTableLen);
                const bool empty = text.empty() && i == 0;
                if (empty || text == encodeWaveTable(t, kWaveTableLen)) { activePreset_ = i; break; }
            }
        repaint();
    }

    void push() {
        cachedText_ = encodeWaveFrames(&frames_[0][0], frameCount_, kWaveTableLen);
        host_.setParamText(name_, pn_, cachedText_);
    }

    void poll() override {
        if (drawing_) return;
        const auto text = host_.liveParamText(name_, pn_);
        if (text != cachedText_) pull(text);
        const int wm = (int) host_.liveParamValue(name_, "WarpMode");
        const float wa = (float) host_.liveParamValue(name_, "Warp");
        const double pos = host_.liveParamValue(name_, "Position");
        if (wm != lastWm_ || std::abs(wa - lastWa_) > 1e-3f || std::abs(pos - pos_) > 1e-3) {
            lastWm_ = wm; lastWa_ = wa; pos_ = pos;
            repaint();
        }
    }

    static constexpr int kMaxFrames = 16;
    std::string pn_;
    float frames_[kMaxFrames][kWaveTableLen] = {};
    int frameCount_ = 1;
    double pos_ = 0.0;
    std::string cachedText_;
    int activePreset_ = -1;
    int lastWm_ = -1;
    float lastWa_ = -1.0f;
    bool drawing_ = false;
    int lastIdx_ = -1;
    float lastVal_ = 0.0f;
    std::unique_ptr<juce::FileChooser> chooser_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveDrawBrick)
};

}
