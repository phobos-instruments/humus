// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/bricks/ClipGridBrick.h"

#include <cmath>

namespace hum {

ClipGridBrick::ClipGridBrick(BrickHost& host, std::string organism, const Bindings& bound)
    : PolledBrick(host, std::move(organism), 2), p_{bound(bind::kFilePrefix), bound(bind::kInPrefix), bound(bind::kOutPrefix), bound(bind::kLoopPrefix), bound(bind::kLaunchPrefix)} {
    for (int i = 0; i < VideoPadSource::kMaxClips; ++i) {
        auto& cell = cells_[(size_t) i];
        cell = std::make_unique<ClipCell>(i);
        cell->node = name_;
        addAndMakeVisible(*cell);
        cell->onLaunch = [this, i] { launch(i); };
        cell->onPlayPause = [this, i] { playPause(i); };
        cell->onSetIn = [this, i] { stamp(i, p_.in); };
        cell->onSetOut = [this, i] { stamp(i, p_.out); };
        cell->onLoop = [this, i] {
            host_.setParam(name_, p_.loop + suffix(i),
                           host_.liveParamValue(name_, p_.loop + suffix(i)) >= 0.5 ? 0.0 : 1.0);
        };
        cell->onOpen = [this, i] { choose(i); };
        cell->onDragIn = [this, i](double seconds) {
            host_.setParam(name_, p_.in + suffix(i), std::round(seconds * 100.0) / 100.0);
        };
        cell->onDragOut = [this, i](double seconds) {
            host_.setParam(name_, p_.out + suffix(i), std::round(seconds * 100.0) / 100.0);
        };
        cell->onScrub = [this, i](double seconds) {
            parkWant_[(size_t) i] = -1.0;
            if (auto l = layer(i)) l->seekSeconds(seconds);
        };
        cell->onDrop = [this, i](const juce::File& f) {
            host_.setParamText(name_, p_.file + suffix(i), f.getFullPathName().toStdString());
        };
        cell->onMenu = [this, i](juce::Point<int> at) { padMenu(i, at); };
        cell->onDropPad = [this, i](int from, bool move) { copyPad(from, i, move); };
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
        parkWant_[(size_t) i] = -1.0;
        return;
    }
    const auto path = file.getFullPathName();
    const double in = std::max(0.0, host_.liveParamValue(name_, p_.in + suffix(i)));
    const bool sameTape = h != nullptr && heldPath_[(size_t) i] == path;
    if (!sameTape) {
        heldPath_[(size_t) i] = path;
        const bool fresh = layer(i) == nullptr;
        h = VideoDeckPool::instance().open(name_ + "/" + suffix(i), path);
        if (h != nullptr && fresh && !onStage) h->setPaused(true);
        armPark(i, in);
    } else if (std::abs(parkIn_[(size_t) i] - in) > 1.0e-6) {
        armPark(i, in);
    }
    if (onStage) parkWant_[(size_t) i] = -1.0;
    else park(i);
}

void ClipGridBrick::armPark(int i, double seconds) {
    parkIn_[(size_t) i] = seconds;
    parkWant_[(size_t) i] = seconds;
    parkTries_[(size_t) i] = 0;
}

void ClipGridBrick::park(int i) {
    const double want = parkWant_[(size_t) i];
    if (want < 0.0 || ++parkTries_[(size_t) i] > kParkTries) return;
    auto l = layer(i);
    if (l == nullptr) return;
    const auto f = l->latestFrame();
    if (f != nullptr && f->pts >= 0.0 && std::abs(f->pts - want) < 0.2) {
        parkWant_[(size_t) i] = -1.0;
        return;
    }
    l->chase(want, 0.0);
}

void ClipGridBrick::launch(int i) {
    host_.setParam(name_, p_.launch + suffix(i), 1.0);
    release_[(size_t) i] = true;
}

void ClipGridBrick::playPause(int i) {
    auto* clips = live<VideoPadSource>();
    auto l = layer(i);
    if (clips != nullptr && clips->clipState().active == i && l != nullptr)
        l->setPaused(!l->isPaused());
    else
        launch(i);
}

void ClipGridBrick::stamp(int i, const std::string& which) {
    auto l = layer(i);
    if (l == nullptr) return;
    host_.setParam(name_, which + suffix(i), std::round(l->positionSeconds() * 100.0) / 100.0);
}

void ClipGridBrick::padMenu(int i, juce::Point<int> at) {
    const bool loaded = !host_.liveParamText(name_, p_.file + suffix(i)).empty();
    juce::PopupMenu m;
    m.addItem(1, tr("clip-grid.clear-pad", "Clear pad"), loaded);
    m.addItem(2, tr("clip-grid.loop", "Loop"), loaded, host_.liveParamValue(name_, p_.loop + suffix(i)) >= 0.5);
    m.addSeparator();
    m.addItem(3, tr("clip-grid.launch-control", "Launch control..."));
    m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({at.x, at.y, 1, 1}),
                    [this, i, at](int r) {
                        if (r == 1) clearPad(i);
                        else if (r == 2)
                            host_.setParam(name_, p_.loop + suffix(i),
                                           host_.liveParamValue(name_, p_.loop + suffix(i)) >= 0.5
                                               ? 0.0 : 1.0);
                        else if (r == 3)
                            showAutomateMenu(host_, name_, p_.launch + suffix(i), at, {});
                    });
}

void ClipGridBrick::clearPad(int i) {
    host_.beginTransaction();
    host_.setParamText(name_, p_.file + suffix(i), {});
    host_.setParam(name_, p_.in + suffix(i), 0.0);
    host_.setParam(name_, p_.out + suffix(i), 0.0);
    host_.setParam(name_, p_.loop + suffix(i), 1.0);
    host_.endTransaction();
}

void ClipGridBrick::copyPad(int a, int b, bool move) {
    if (a == b || a < 0 || b < 0 || a >= VideoPadSource::kMaxClips
        || b >= VideoPadSource::kMaxClips)
        return;
    struct Pad { std::string file; double in, out, loop; };
    auto read = [this](int i) {
        return Pad{host_.liveParamText(name_, p_.file + suffix(i)),
                   host_.liveParamValue(name_, p_.in + suffix(i)),
                   host_.liveParamValue(name_, p_.out + suffix(i)),
                   host_.liveParamValue(name_, p_.loop + suffix(i))};
    };
    auto writeRange = [this](int i, const Pad& pad) {
        host_.setParam(name_, p_.in + suffix(i), pad.in);
        host_.setParam(name_, p_.out + suffix(i), pad.out);
        host_.setParam(name_, p_.loop + suffix(i), pad.loop);
    };
    const Pad from = read(a), to = read(b);
    host_.beginTransaction();
    host_.setParamText(name_, p_.file + suffix(b), from.file);
    if (move) host_.setParamText(name_, p_.file + suffix(a), to.file);
    writeRange(b, from);
    if (move) writeRange(a, to);
    host_.endTransaction();
}

void ClipGridBrick::choose(int i) {
    const auto cur = juce::String(host_.liveParamText(name_, p_.file + suffix(i)));
    const auto start = cur.isNotEmpty()
                           ? juce::File(cur).getParentDirectory()
                           : juce::File::getSpecialLocation(juce::File::userMoviesDirectory);
    chooser_ = std::make_unique<juce::FileChooser>("Load a clip", start,
                                                   "*.mov;*.mp4;*.m4v;*.avi");
    chooser_->launchAsync(juce::FileBrowserComponent::openMode
                              | juce::FileBrowserComponent::canSelectFiles,
                          [this, i](const juce::FileChooser& fc) {
                              const auto f = fc.getResult();
                              if (f == juce::File()) return;
                              host_.setParamText(name_, p_.file + suffix(i),
                                                 f.getFullPathName().toStdString());
                          });
}

juce::Image ClipGridBrick::thumbnailOf(const VideoLayer::Frame& f) const {
    return imageOfFrame(f, (int) std::lround(ClipCell::kWidth * scale_),
                        (int) std::lround(ClipCell::kThumbH * scale_));
}

void ClipGridBrick::takeClip(int i, const ClipEditor::MediaRange& r) {
    if (r.file.empty()) return;
    host_.setParamText(name_, p_.file + suffix(i), r.file);
    host_.setParam(name_, p_.in + suffix(i), std::round(r.inSeconds * 100.0) / 100.0);
    host_.setParam(name_, p_.out + suffix(i), std::round(r.outSeconds * 100.0) / 100.0);
    host_.setParam(name_, p_.loop + suffix(i), r.looped ? 1.0 : 0.0);
}

void ClipGridBrick::poll() {
    VideoPadSource::ClipState st;
    auto* clips = live<VideoPadSource>();
    if (clips != nullptr) st = clips->clipState();
    const int pads = clips != nullptr ? clips->clipCount() : VideoPadSource::kMaxClips;
    for (int i = 0; i < VideoPadSource::kMaxClips; ++i) {
        cells_[(size_t) i]->setVisible(i < pads);
        if (release_[(size_t) i]) {
            release_[(size_t) i] = false;
            host_.setParam(name_, p_.launch + suffix(i), 0.0);
        }
        const auto path = juce::String(host_.liveParamText(name_, p_.file + suffix(i)));
        const auto file = VideoDeckPool::resolveTape(host_.documentPath(), path);
        hold(i, file, st.active == i || st.outgoing == i);
        auto l = layer(i);
        const double len = l != nullptr ? l->lengthSeconds() : 0.0;
        if (clips != nullptr) clips->noteClipLength(i, len);
        auto& cell = *cells_[(size_t) i];
        cell.setState(file == juce::File() ? juce::String() : file.getFileName(),
                      st.active == i, st.outgoing == i, l != nullptr && l->isPaused(),
                      l != nullptr ? l->positionSeconds() : 0.0, len,
                      host_.liveParamValue(name_, p_.in + suffix(i)),
                      host_.liveParamValue(name_, p_.out + suffix(i)),
                      host_.liveParamValue(name_, p_.loop + suffix(i)) >= 0.5);
        auto frame = l != nullptr ? l->latestFrame() : nullptr;
        if (frame != shown_[(size_t) i]) {
            shown_[(size_t) i] = frame;
            cell.setThumbnail(frame != nullptr ? thumbnailOf(*frame) : juce::Image());
        }
    }
}

}
