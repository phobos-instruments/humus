// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/bricks/WaveDrawBrick.h"

#include <cmath>
#include <cstdint>
#include <vector>

#include "core/app/AppPaths.h"
#include "hum/FileBytes.h"
#include "hum/dsp/WaveTableFile.h"

namespace hum {

WaveDrawBrick::WaveDrawBrick(BrickHost& host, std::string organism, std::string param, const Bindings& bound)
    : PolledBrick(host, organism, 4),
      wave_(host, organism, {std::move(param), bound(bind::kPosition), bound(bind::kWarp), bound(bind::kWarpMode)}) {
    repaint();
}

void WaveDrawBrick::paint(juce::Graphics& g) {
    const auto area = waveArea().toFloat();
    g.setColour(Palette::background);
    g.fillRoundedRectangle(area, 4.0f);
    const float cy = area.getCentreY();
    g.setColour(Palette::border.withAlpha(alpha::muted));
    for (int q = 1; q < 4; ++q)
        g.drawVerticalLine((int) (area.getX() + area.getWidth() * (float) q / 4.0f),
                           area.getY() + 2, area.getBottom() - 2);
    g.drawHorizontalLine((int) cy, area.getX() + 2, area.getRight() - 2);

    juce::Path p;
    const int steps = juce::jmax(64, (int) area.getWidth());
    p.startNewSubPath(area.getX(), cy);
    for (int i = 0; i <= steps; ++i) {
        const double ph = (double) i / steps;
        const float v = wave_.valueAt(ph);
        p.lineTo(area.getX() + (float) ph * area.getWidth(),
                 cy - v * area.getHeight() * 0.46f);
    }
    p.lineTo(area.getRight(), cy);
    p.closeSubPath();
    g.setColour(Palette::accent.withAlpha(alpha::scrim));
    g.fillPath(p);
    g.setColour(Palette::accent);
    g.strokePath(p, juce::PathStrokeType(2.0f));

    const int wm = wave_.warpMode();
    const float wa = wave_.warpAmount();
    if (wm > 0 && wa > 0.0f) {
        static thread_local float disp[kWaveTableLen];
        wave_.blended(disp);
        juce::Path wp;
        for (int i = 0; i <= steps; ++i) {
            const double ph = (double) i / steps;
            const float v = warpRead(disp, kWaveTableLen, ph, wm, wa);
            const float y = cy - juce::jlimit(-1.0f, 1.0f, v) * area.getHeight() * 0.46f;
            if (i == 0) wp.startNewSubPath(area.getX(), y);
            else wp.lineTo(area.getX() + (float) ph * area.getWidth(), y);
        }
        g.setColour(Palette::text.withAlpha(alpha::strong));
        g.strokePath(wp, juce::PathStrokeType(1.5f));
    }

    g.setColour(dropping_ ? Palette::accent : Palette::border);
    g.drawRoundedRectangle(area, 4.0f, dropping_ ? 2.0f : 1.0f);

    g.setColour(dropping_ ? Palette::accent : Palette::textDim);
    g.setFont(juce::FontOptions(11.0f));
    g.drawText(dropping_ ? tr("wave-draw.drop-to-seed", "drop to seed") : sourceText(),
               area.toNearestInt().reduced(8, 6), juce::Justification::topRight, false);

    paintStrip(g);

    for (int i = 0; i < kWaveTablePresets; ++i) {
        const auto r = tileRect(i).toFloat();
        const bool on = i == wave_.activePreset();
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
        g.setColour(wave_.frameCount() >= (t == 2 ? kMaxFrames : 2) ? Palette::textDim : Palette::text);
        g.setFont(juce::FontOptions(15.0f));
        g.drawText(t == 2 ? tr("wave-draw.frame", "+ frame") : tr("wave-draw.frame-2", "- frame"), r, juce::Justification::centred);
    }
}

bool WaveDrawBrick::isInterestedInFileDrag(const juce::StringArray& files) {
    return files.size() == 1 && juce::File(files[0]).existsAsFile();
}

void WaveDrawBrick::fileDragEnter(const juce::StringArray&, int, int) {
    dropping_ = true;
    repaint();
}

void WaveDrawBrick::fileDragExit(const juce::StringArray&) {
    dropping_ = false;
    repaint();
}

void WaveDrawBrick::filesDropped(const juce::StringArray& files, int, int) {
    dropping_ = false;
    if (!files.isEmpty()) seedFromFile(juce::File(files[0]));
    repaint();
}

juce::String WaveDrawBrick::sourceText() const { return juce::String(wave_.sourceText()); }

void WaveDrawBrick::mouseDown(const juce::MouseEvent& e) {
    for (int i = 0; i < kWaveTablePresets; ++i)
        if (tileRect(i).contains(e.getPosition())) {
            wave_.choosePreset(i);
            repaint();
            return;
        }
    if (tileRect(kWaveTablePresets).contains(e.getPosition())) {
        wave_.randomize((std::uint32_t) juce::Random::getSystemRandom().nextInt());
        repaint();
        return;
    }
    if (tileRect(kWaveTablePresets + 1).contains(e.getPosition())) {
        chooseSeed();
        return;
    }
    if (tileRect(kWaveTablePresets + 2).contains(e.getPosition())) {
        if (wave_.addFrame()) repaint();
        return;
    }
    if (tileRect(kWaveTablePresets + 3).contains(e.getPosition())) {
        if (wave_.removeFrame()) repaint();
        return;
    }
    if (wave_.frameCount() > 1 && stripRect().contains(e.getPosition())) {
        wave_.pickFrame(e.x, stripRect().getX(), stripRect().getWidth());
        return;
    }
    if (waveArea().contains(e.getPosition())) {
        wave_.beginDrawing();
        paintSample(e);
    }
}

void WaveDrawBrick::mouseDrag(const juce::MouseEvent& e) {
    if (wave_.drawing()) paintSample(e);
}

void WaveDrawBrick::mouseUp(const juce::MouseEvent&) {
    if (wave_.endDrawing()) repaint();
}

void WaveDrawBrick::mouseMove(const juce::MouseEvent& e) {
    setMouseCursor(waveArea().contains(e.getPosition())
                       ? juce::MouseCursor::CrosshairCursor
                       : juce::MouseCursor::NormalCursor);
}

juce::Rectangle<int> WaveDrawBrick::stripRect() const {
    auto r = getLocalBounds();
    r.removeFromBottom(kTileH + kTileGap);
    return r.removeFromBottom(kStripH);
}

juce::Rectangle<int> WaveDrawBrick::waveArea() const {
    auto r = getLocalBounds();
    r.removeFromBottom(kTileH + kTileGap + kStripH + kStripGap);
    return r;
}

juce::Rectangle<int> WaveDrawBrick::frameSlot(int f) const {
    const auto s = stripRect();
    const int n = std::max(1, wave_.frameCount());
    const int w = s.getWidth() / n;
    return {s.getX() + f * w, s.getY(), w - 1, s.getHeight()};
}

juce::Rectangle<int> WaveDrawBrick::tileRect(int i) const {
    const int w = (getWidth() - (kTiles - 1) * kTileGap) / kTiles;
    return {i * (w + kTileGap), getHeight() - kTileH, w, kTileH};
}

void WaveDrawBrick::paintSample(const juce::MouseEvent& e) {
    const auto area = waveArea();
    wave_.drawAt(e.x, e.y, area.getX(), area.getWidth(), area.getCentreY(), area.getHeight());
    repaint();
}

void WaveDrawBrick::paintStrip(juce::Graphics& g) {
    const auto s = stripRect();
    g.setColour(Palette::background.darker(0.1f));
    g.fillRoundedRectangle(s.toFloat(), 3.0f);
    const int frameCount = wave_.frameCount();
    if (frameCount <= 1) {
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(10.0f));
        g.drawText(tr("wave-draw.one-frame", "one frame"), s, juce::Justification::centred);
        return;
    }
    const int cur = wave_.currentFrame();
    for (int f = 0; f < frameCount; ++f) {
        const auto slot = frameSlot(f).toFloat().reduced(1.0f, 2.0f);
        if (f == cur) {
            g.setColour(Palette::accent.withAlpha(alpha::scrim));
            g.fillRect(slot);
        }
        juce::Path mp;
        const int steps = std::max(8, (int) slot.getWidth());
        const float cy = slot.getCentreY();
        for (int k = 0; k <= steps; ++k) {
            const double ph = (double) k / steps;
            const int idx = juce::jlimit(0, kWaveTableLen - 1, (int) (ph * (kWaveTableLen - 1)));
            const float y = cy - wave_.frame(f)[idx] * slot.getHeight() * 0.42f;
            const juce::Point<float> pt(slot.getX() + (float) ph * slot.getWidth(), y);
            if (k == 0) mp.startNewSubPath(pt); else mp.lineTo(pt);
        }
        g.setColour(f == cur ? Palette::accent : Palette::text.withAlpha(alpha::dim));
        g.strokePath(mp, juce::PathStrokeType(1.0f));
    }
}

void WaveDrawBrick::chooseSeed() {
    files::FilePick request;
    request.title = "Seed a wave from any file";
    request.patterns = "*";
    picker_.pick(request, [this](const std::vector<std::string>& paths) {
        if (!paths.empty()) seedFromFile(juce::File(juce::String::fromUTF8(paths.front().c_str())));
    });
}

void WaveDrawBrick::seedFromFile(const juce::File& f) {
    if (const auto img = juce::ImageFileFormat::loadFrom(f); img.isValid()) {
        const int w = juce::jmin(img.getWidth(), 4096);
        std::vector<float> row((size_t) w);
        const int y = img.getHeight() / 2;
        for (int x = 0; x < w; ++x) row[(size_t) x] = img.getPixelAt(x, y).getBrightness() * 2.0f - 1.0f;
        wave_.seedCycle(row.data(), w);
    } else if (loadAudioSeed(f)) {
    } else if (signalfile::Signal sig;
               signalfile::load(f, sig) && wave_.seedSignal(sig.samples, signalfile::kindName(sig.kind), sig.label)) {
    } else if (juce::FileInputStream in(f); in.openedOk() && in.getTotalLength() > 0) {
        const auto size = in.getTotalLength();
        const int take = (int) juce::jmin(size, (juce::int64) 65536);
        in.setPosition((size - take) / 2);
        std::vector<uint8_t> bytes((size_t) take);
        in.read(bytes.data(), take);
        wave_.seedBytes(bytes.data(), take);
    } else {
        return;
    }
    repaint();
}

bool WaveDrawBrick::loadAudioSeed(const juce::File& f) {
    juce::AudioBuffer<float> buf;
    double fileSr = 0.0;
    if (!loadSoundFile(f.getFullPathName().toStdString(), buf, fileSr) || buf.getNumSamples() < 64) return false;
    const int n = buf.getNumSamples();
    std::vector<std::uint8_t> bytes;
    int frameLen = 0;
    if (n <= kWaveFileFrameLen * kWaveFileMaxFrames && readFileBytes(pathOf(f), bytes))
        frameLen = waveTableFrameLenFromRiff(bytes.data(), bytes.size());
    const bool wholeFile = frameLen > 0 || n <= kWaveFileFrameLen * kWaveFileMaxFrames;
    const int take = wholeFile ? n : juce::jmin(n, (int) (2.0 * fileSr));
    const int start = (n - take) / 2;
    std::vector<float> mono((size_t) take, 0.0f);
    for (int c = 0; c < buf.getNumChannels(); ++c) {
        const float* src = buf.getReadPointer(c, start);
        for (int i = 0; i < take; ++i) mono[(size_t) i] += src[i] / (float) buf.getNumChannels();
    }
    if (frameLen == 0) frameLen = waveTableFrameLenGuess(mono.data(), take);
    if (frameLen > 0) wave_.seedTable(mono, frameLen);
    else wave_.seedAudio(mono, fileSr);
    return true;
}

void WaveDrawBrick::poll() {
    if (wave_.poll()) repaint();
}

}
