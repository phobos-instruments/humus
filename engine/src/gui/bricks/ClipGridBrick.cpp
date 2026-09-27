// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/bricks/ClipGridBrick.h"

#include <cmath>

#include "core/app/AppPaths.h"
#include "gui/app/StartDirs.h"

namespace hum {

ClipGridBrick::ClipGridBrick(BrickHost& host, std::string organism, const Bindings& bound)
    : PolledBrick(host, organism, 2),
      pads_(host, organism,
            {bound(bind::kFilePrefix), bound(bind::kInPrefix), bound(bind::kOutPrefix), bound(bind::kLoopPrefix),
             bound(bind::kLaunchPrefix)}) {
    for (int i = 0; i < VideoPadSource::kMaxClips; ++i) {
        auto& cell = cells_[(size_t) i];
        cell = std::make_unique<ClipCell>(i);
        cell->node = name_;
        addAndMakeVisible(*cell);
        cell->onLaunch = [this, i] { pads_.launch(i); };
        cell->onPlayPause = [this, i] {
            JuceVideoTarget t(layer(i));
            pads_.playPause(i, t.get());
        };
        cell->onSetIn = [this, i] {
            JuceVideoTarget t(layer(i));
            pads_.stampIn(i, t.get());
        };
        cell->onSetOut = [this, i] {
            JuceVideoTarget t(layer(i));
            pads_.stampOut(i, t.get());
        };
        cell->onLoop = [this, i] { pads_.toggleLoop(i); };
        cell->onOpen = [this, i] { choose(i); };
        cell->onDragIn = [this, i](double seconds) { pads_.setIn(i, seconds); };
        cell->onDragOut = [this, i](double seconds) { pads_.setOut(i, seconds); };
        cell->onScrub = [this, i](double seconds) {
            pads_.forgetPark(i);
            if (auto l = layer(i)) l->seekSeconds(seconds);
        };
        cell->onDrop = [this, i](const juce::File& f) { pads_.load(i, f.getFullPathName().toStdString()); };
        cell->onMenu = [this, i](juce::Point<int> at) { padMenu(i, at); };
        cell->onDropPad = [this, i](int from, bool move) { pads_.copy(from, i, move); };
        cell->onDropClip = [this, i](const std::string& track, int clipId) {
            takeClip(i, host_.clips().rangeOf(track, clipId));
        };
    }
    poll();
}

int ClipGridBrick::preferredContentHeight(int) const {
    return kRows * ClipCell::kHeight + (kRows - 1) * kGap;
}

void ClipGridBrick::resized() {
    const double want = (double) getWidth() / std::max(1, preferredContentWidth());
    const double s = juce::jlimit(0.5, kMaxScale, want > 0.0 ? want : 1.0);
    if (std::abs(s - scale_) > 1.0e-6) {
        scale_ = s;
        for (auto& f : shown_) f.reset();
    }
    for (int i = 0; i < VideoPadSource::kMaxClips; ++i) {
        auto& cell = *cells_[(size_t) i];
        cell.setBounds(0, 0, ClipCell::kWidth, ClipCell::kHeight);
        cell.setTransform(juce::AffineTransform::scale((float) s).translated(
            (float) ((i % kCols) * (ClipCell::kWidth + kGap) * s),
            (float) ((i / kCols) * (ClipCell::kHeight + kGap) * s)));
    }
}

std::shared_ptr<VideoLayer> ClipGridBrick::layer(int i) const {
    return VideoDeckPool::instance().peek(name_ + "/" + suffix(i));
}

void ClipGridBrick::hold(int i, const juce::File& file, bool onStage) {
    auto& h = held_[(size_t) i];
    if (file == juce::File()) {
        h.reset();
        heldPath_[(size_t) i] = {};
        pads_.forgetPark(i);
        return;
    }
    const auto path = file.getFullPathName();
    const double in = std::max(0.0, pads_.in(i));
    const bool sameTape = h != nullptr && heldPath_[(size_t) i] == path;
    if (!sameTape) {
        heldPath_[(size_t) i] = path;
        const bool fresh = layer(i) == nullptr;
        h = VideoDeckPool::instance().open(name_ + "/" + suffix(i), path);
        if (h != nullptr && fresh && !onStage) h->setPaused(true);
        pads_.armPark(i, in);
    } else if (pads_.parkMoved(i, in)) {
        pads_.armPark(i, in);
    }
    if (onStage) pads_.forgetPark(i);
    else park(i);
}

void ClipGridBrick::park(int i) {
    auto l = layer(i);
    const auto f = l != nullptr ? l->latestFrame() : nullptr;
    const double want = pads_.parkWant(i);
    if (pads_.parkStep(i, f != nullptr, f != nullptr ? f->pts : -1.0) == video::ClipPadsModel::Park::Chase
        && l != nullptr)
        l->chase(want, 0.0);
}

void ClipGridBrick::padMenu(int i, juce::Point<int> at) {
    const bool loaded = !pads_.file(i).empty();
    juce::PopupMenu m;
    m.addItem(1, tr("clip-grid.clear-pad", "Clear pad"), loaded);
    m.addItem(2, tr("clip-grid.loop", "Loop"), loaded, pads_.looped(i));
    m.addSeparator();
    m.addItem(3, tr("clip-grid.launch-control", "Launch control..."));
    m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({at.x, at.y, 1, 1}),
                    [this, i, at](int r) {
                        if (r == 1) pads_.clear(i);
                        else if (r == 2) pads_.toggleLoop(i);
                        else if (r == 3)
                            showAutomateMenu(host_, name_, pads_.launchParam() + suffix(i), at, {});
                    });
}

void ClipGridBrick::choose(int i) {
    const auto key = mediaFolderKey("Videos");
    auto from = keptFolder(key);
    if (!from.isDirectory()) from = juce::File::getSpecialLocation(juce::File::userMoviesDirectory);
    picker_.pick(pads_.request(i, from.getFullPathName().toStdString()),
                 [this, i, key](const std::vector<std::string>& paths) {
        if (paths.empty()) return;
        keepFolder(key, fileAt(paths.front()));
        pads_.load(i, paths.front());
    });
}

juce::Image ClipGridBrick::thumbnailOf(const VideoLayer::Frame& f) const {
    return imageOfFrame(f, (int) std::lround(ClipCell::kWidth * scale_),
                        (int) std::lround(ClipCell::kThumbH * scale_));
}

void ClipGridBrick::takeClip(int i, const ClipEditor::MediaRange& r) {
    pads_.take(i, {r.file, r.inSeconds, r.outSeconds, r.looped});
}

void ClipGridBrick::poll() {
    VideoPadSource::ClipState st;
    auto* clips = live<VideoPadSource>();
    if (clips != nullptr) st = clips->clipState();
    const int pads = clips != nullptr ? clips->clipCount() : VideoPadSource::kMaxClips;
    for (int i = 0; i < VideoPadSource::kMaxClips; ++i) {
        cells_[(size_t) i]->setVisible(i < pads);
        pads_.releaseLaunch(i);
        const auto path = juce::String(pads_.file(i));
        const auto file = VideoDeckPool::resolveTape(host_.documentPath(), path);
        hold(i, file, st.active == i || st.outgoing == i);
        auto l = layer(i);
        const double len = l != nullptr ? l->lengthSeconds() : 0.0;
        if (clips != nullptr) clips->noteClipLength(i, len);
        auto& cell = *cells_[(size_t) i];
        cell.setState(file == juce::File() ? juce::String() : file.getFileName(),
                      st.active == i, st.outgoing == i, l != nullptr && l->isPaused(),
                      l != nullptr ? l->positionSeconds() : 0.0, len,
                      pads_.in(i), pads_.out(i), pads_.looped(i));
        auto frame = l != nullptr ? l->latestFrame() : nullptr;
        if (frame != shown_[(size_t) i]) {
            shown_[(size_t) i] = frame;
            cell.setThumbnail(frame != nullptr ? thumbnailOf(*frame) : juce::Image());
        }
    }
}

}
