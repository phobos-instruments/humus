// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <cmath>
#include <memory>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/timeline/ClipDrag.h"
#include "gui/editor/AutomateMenu.h"
#include "gui/style/Colours.h"
#include "gui/editor/BrickBindings.h"
#include "gui/host/BrickHost.h"
#include "gui/host/EngineHostClips.h"
#include "gui/video/FrameImage.h"
#include "gui/style/IconGlyph.h"
#include "gui/style/LookAndFeel.h"
#include "gui/properties/PadBar.h"
#include "gui/editor/OrganismEditor.h"
#include "gui/bricks/PolledBrick.h"
#include "gui/video/VideoDeckPool.h"
#include "hum/caps/Video.h"
#include "gui/common/Localisation.h"
#include "gui/bricks/ClipCell.h"

namespace hum {

class ClipGridBrick : public PolledBrick {
public:
    static constexpr int kCols = 4, kGap = 8, kParkTries = 200;
    static constexpr double kMaxScale = 2.5;
    static constexpr int kRows = VideoPadSource::kMaxClips / kCols;

    ClipGridBrick(BrickHost& host, std::string organism, const Bindings& bound);

    void reloadValues() override { poll(); }
    int preferredContentWidth() const override { return kCols * ClipCell::kWidth + (kCols - 1) * kGap; }
    int preferredContentHeight(int) const override;

    void resized() override;

private:
    struct PadParams { std::string file, in, out, loop, launch; };
    PadParams p_;

    static std::string suffix(int i) { return std::to_string(i + 1); }

    std::shared_ptr<VideoLayer> layer(int i) const;

    void hold(int i, const juce::File& file, bool onStage);

    void armPark(int i, double seconds);

    void park(int i);

    void launch(int i);

    void playPause(int i);

    void stamp(int i, const std::string& which);

    void padMenu(int i, juce::Point<int> at);

    void clearPad(int i);

    void copyPad(int a, int b, bool move);

    void choose(int i);

    juce::Image thumbnailOf(const VideoLayer::Frame& f) const;

    void takeClip(int i, const ClipEditor::MediaRange& r);

    void poll() override;

    std::array<std::unique_ptr<ClipCell>, VideoPadSource::kMaxClips> cells_;
    std::array<std::shared_ptr<const VideoLayer::Frame>, VideoPadSource::kMaxClips> shown_;
    std::array<bool, VideoPadSource::kMaxClips> release_{};
    std::array<std::shared_ptr<VideoLayer>, VideoPadSource::kMaxClips> held_;
    std::array<juce::String, VideoPadSource::kMaxClips> heldPath_;
    std::array<double, VideoPadSource::kMaxClips> parkWant_{};
    std::array<double, VideoPadSource::kMaxClips> parkIn_{};
    std::array<int, VideoPadSource::kMaxClips> parkTries_{};
    std::unique_ptr<juce::FileChooser> chooser_;
    double scale_ = 1.0;
};

}
