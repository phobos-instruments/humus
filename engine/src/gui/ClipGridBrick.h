#pragma once
#include <array>
#include <cmath>
#include <memory>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/ClipDrag.h"
#include "gui/EngineHost.h"
#include "gui/FrameImage.h"
#include "gui/IconGlyph.h"
#include "gui/LookAndFeel.h"
#include "gui/PadBar.h"
#include "gui/OrganismEditor.h"
#include "gui/PolledBrick.h"
#include "gui/VideoDeckPool.h"
#include "hum/Capabilities.h"
#include "gui/Localisation.h"

namespace hum {

class CellButton : public juce::Button {
public:
    CellButton(IconGlyph glyph, const juce::String& text)
        : juce::Button(text.isEmpty() ? juce::String(kIconGlyphNames[(size_t) glyph]) : text),
          glyph_(glyph), text_(text) {}

    void setGlyph(IconGlyph g) {
        if (g == glyph_) return;
        glyph_ = g;
        repaint();
    }

    void paintButton(juce::Graphics& g, bool over, bool down) override {
        auto r = getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(Palette::panel.brighter(down ? 0.20f : over ? 0.12f : 0.05f));
        g.fillRoundedRectangle(r, 4.0f);
        g.setColour(getToggleState() ? Palette::accent : Palette::border);
        g.drawRoundedRectangle(r.reduced(0.5f), 4.0f, 1.0f);
        const auto tint = !isEnabled() ? Palette::textDim.withAlpha(0.4f)
                          : getToggleState() ? Palette::accent
                                             : Palette::text;
        if (text_.isNotEmpty()) {
            g.setColour(tint);
            g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
            g.drawText(text_, getLocalBounds(), juce::Justification::centred, false);
            return;
        }
        drawIconGlyph(g, glyph_, r.reduced(r.getHeight() * 0.28f), tint, isEnabled(),
                      getToggleState());
    }

private:
    IconGlyph glyph_;
    juce::String text_;
};

class ClipCell : public juce::Component,
                 public juce::FileDragAndDropTarget,
                 public juce::DragAndDropTarget {
public:
    static constexpr int kWidth = 138, kThumbH = 78, kHeight = 118;

    std::function<void()> onLaunch, onPlayPause, onSetIn, onSetOut, onLoop, onOpen;
    std::function<void(double)> onScrub;
    std::function<void(const juce::File&)> onDrop;
    std::function<void(juce::Point<int>)> onMenu;
    std::function<void(int, bool)> onDropPad;
    std::function<void(double)> onDragIn, onDragOut;
    std::function<void(const std::string&, int)> onDropClip;
    std::string node;

    explicit ClipCell(int row) : row_(row) {
        for (auto* b : {&open_, &play_, &in_, &out_, &loop_}) addAndMakeVisible(*b);
        open_.onClick = [this] { if (onOpen) onOpen(); };
        play_.onClick = [this] { if (onPlayPause) onPlayPause(); };
        in_.onClick = [this] { if (onSetIn) onSetIn(); };
        out_.onClick = [this] { if (onSetOut) onSetOut(); };
        loop_.onClick = [this] { if (onLoop) onLoop(); };
        open_.setTooltip(tr("clip-grid.load-a-clip", "Load a clip"));
        play_.setTooltip(tr("clip-grid.play-pause", "Play / pause"));
        in_.setTooltip(tr("clip-grid.set-the-in-point-here", "Set the In point here"));
        out_.setTooltip(tr("clip-grid.set-the-out-point-here", "Set the Out point here"));
        loop_.setTooltip(tr("clip-grid.loop-between-in-and-out", "Loop between In and Out"));
    }

    void setState(const juce::String& fileName, bool active, bool outgoing, bool paused,
                  double pos, double len, double in, double out, bool loop) {
        fileName_ = fileName;
        active_ = active;
        outgoing_ = outgoing;
        pos_ = pos;
        len_ = len;
        inSec_ = in;
        outSec_ = out;
        const bool loaded = fileName.isNotEmpty();
        const bool staged = len > 0.0;
        loop_.setToggleState(loop, juce::dontSendNotification);
        play_.setToggleState(active && !paused, juce::dontSendNotification);
        play_.setGlyph(active && !paused ? IconGlyph::Pause : IconGlyph::Play);
        play_.setEnabled(loaded);
        in_.setEnabled(staged);
        out_.setEnabled(staged);
        loop_.setEnabled(loaded);
        repaint();
    }

    void setThumbnail(juce::Image img) {
        thumb_ = std::move(img);
        repaint();
    }

    void resized() override {
        auto row = juce::Rectangle<int>(0, kThumbH + 4, kWidth, 22);
        const int bw = 26;
        for (auto* b : {&open_, &play_, &in_, &out_, &loop_}) {
            b->setBounds(row.removeFromLeft(bw));
            row.removeFromLeft(2);
        }
    }

    void paint(juce::Graphics& g) override {
        auto thumb = juce::Rectangle<int>(0, 0, kWidth, kThumbH);
        g.setColour(Palette::panel.darker(0.3f));
        g.fillRoundedRectangle(thumb.toFloat(), 4.0f);
        if (thumb_.isValid()) {
            g.saveState();
            g.reduceClipRegion(thumb);
            g.drawImage(thumb_, thumb.toFloat(), juce::RectanglePlacement::fillDestination);
            g.restoreState();
        } else if (fileName_.isEmpty()) {
            g.setColour(Palette::textDim.withAlpha(0.6f));
            g.setFont(juce::FontOptions(10.0f));
            g.drawText(tr("clip-grid.drop-a-clip", "drop a clip"), thumb, juce::Justification::centred, false);
        }
        if (fileName_.isNotEmpty()) {
            auto band = thumb.withTrimmedTop(kThumbH - padbar::kTapeTop - 13)
                            .withTrimmedBottom(padbar::kTapeTop);
            g.setColour(Palette::background.withAlpha(0.72f));
            g.fillRect(band);
            g.setColour(Palette::text);
            g.setFont(juce::FontOptions(10.0f));
            g.drawText(fileName_, band.reduced(4, 0), juce::Justification::centredLeft, true);
        }
        if (len_ > 0.0) {
            const auto clip = span();
            const auto tape = juce::Rectangle<float>(
                0.0f, (float) (kThumbH - padbar::kTapeTop), (float) kWidth, (float) padbar::kTapeH);
            g.setColour(Palette::background.withAlpha(0.88f));
            g.fillRect(tape.withTop(tape.getY() - 2.0f).withBottom((float) kThumbH));
            const float a = (float) padbar::xOfTape(clip.lo, kWidth, len_);
            const float b = (float) padbar::xOfTape(clip.hi, kWidth, len_);
            g.setColour(Palette::accent.withAlpha(0.55f));
            g.fillRect(a, tape.getY(), std::max(1.5f, b - a), tape.getHeight());
            for (const float x : {a, b}) {
                g.setColour(grip_ != padbar::Grip::None ? Palette::accent : Palette::text);
                g.fillRect(juce::jlimit(0.0f, (float) kWidth - 2.0f, x - 1.0f),
                           tape.getY() - 2.0f, 2.0f, tape.getHeight() + 3.0f);
            }
            const auto bar = juce::Rectangle<float>(0.0f, (float) (kThumbH - padbar::kBarH),
                                                    (float) kWidth, (float) padbar::kBarH);
            g.setColour(Palette::accent.withAlpha(0.45f));
            g.fillRect(bar);
            const float p = (float) juce::jlimit(0.0, 1.0, (pos_ - clip.lo) / clip.length())
                          * bar.getWidth();
            g.setColour(Palette::text);
            g.fillRect(juce::jlimit(0.0f, (float) kWidth - 3.0f, p - 1.5f), bar.getY() - 1.0f,
                       3.0f, bar.getHeight() + 1.0f);
        }
        const auto badge = juce::Rectangle<float>(4.0f, 4.0f, 18.0f, 14.0f);
        g.setColour(active_ ? Palette::accent : Palette::background.withAlpha(0.72f));
        g.fillRoundedRectangle(badge, 3.0f);
        g.setColour(active_ ? Palette::background : Palette::text);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText(juce::String(row_ + 1), badge, juce::Justification::centred, false);
        g.setColour(active_ ? Palette::accent
                    : outgoing_ ? Palette::accentDim
                    : dragOver_ ? Palette::text
                                : Palette::border);
        g.drawRoundedRectangle(thumb.toFloat().reduced(0.5f), 4.0f,
                               active_ || dragOver_ ? 2.0f : 1.0f);
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(10.0f));
        g.drawText(rangeText(), juce::Rectangle<int>(0, kThumbH + 28, kWidth, 12),
                   juce::Justification::centred, false);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu() && onMenu) onMenu(e.getScreenPosition());
        dragged_ = false;
        badgeDrag_ = fileName_.isNotEmpty() && e.x < 26 && e.y < 22;
        lifted_ = false;
        grip_ = padbar::gripAt(e.x, e.y, kWidth, kThumbH, span(), len_);
        if (grip_ != padbar::Grip::None) repaint();
    }

    void mouseMove(const juce::MouseEvent& e) override {
        setMouseCursor(padbar::gripAt(e.x, e.y, kWidth, kThumbH, span(), len_) == padbar::Grip::None
                           ? juce::MouseCursor::NormalCursor
                           : juce::MouseCursor::LeftRightResizeCursor);
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu() || e.getMouseDownY() > kThumbH) return;
        if (grip_ != padbar::Grip::None) {
            const double t = padbar::tapeTimeAtX(e.x, kWidth, len_);
            if (grip_ == padbar::Grip::In) {
                if (onDragIn) onDragIn(padbar::dragIn(t, outSec_, len_));
            } else if (onDragOut) {
                onDragOut(padbar::dragOut(t, inSec_, len_));
            }
            return;
        }
        if (badgeDrag_) {
            if (lifted_ || e.getDistanceFromDragStart() < 4) return;
            lifted_ = true;
            if (auto* dc = juce::DragAndDropContainer::findParentDragContainerFor(this))
                dc->startDragging(juce::String(clipdrag::videoPad(node, row_)), this,
                                  juce::ScaledImage(), true);
            return;
        }
        if (len_ <= 0.0) return;
        if (!dragged_ && e.getDistanceFromDragStart() < 3) return;
        dragged_ = true;
        if (onScrub) onScrub(padbar::clipTimeAtX(e.x, kWidth, span()));
    }

    void mouseUp(const juce::MouseEvent& e) override {
        if (grip_ != padbar::Grip::None) {
            grip_ = padbar::Grip::None;
            repaint();
            return;
        }
        if (e.mods.isPopupMenu() || dragged_ || lifted_ || e.y > kThumbH) return;
        if (fileName_.isEmpty()) {
            if (onOpen) onOpen();
            return;
        }
        if (onLaunch) onLaunch();
    }

    bool isInterestedInFileDrag(const juce::StringArray& files) override {
        return files.size() == 1 && accepts(files[0]);
    }
    void fileDragEnter(const juce::StringArray&, int, int) override {
        dragOver_ = true;
        repaint();
    }
    void fileDragExit(const juce::StringArray&) override {
        dragOver_ = false;
        repaint();
    }
    void filesDropped(const juce::StringArray& files, int, int) override {
        dragOver_ = false;
        if (files.size() == 1 && onDrop) onDrop(juce::File(files[0]));
        repaint();
    }

    bool isInterestedInDragSource(const SourceDetails& d) override {
        std::string from;
        int number = 0;
        const auto text = d.description.toString().toStdString();
        if (clipdrag::parseVideoClip(text, from, number)) return true;
        return clipdrag::parseVideoPad(text, from, number) && from == node && number != row_;
    }
    void itemDragEnter(const SourceDetails&) override {
        dragOver_ = true;
        repaint();
    }
    void itemDragExit(const SourceDetails&) override {
        dragOver_ = false;
        repaint();
    }
    void itemDropped(const SourceDetails& d) override {
        dragOver_ = false;
        std::string from;
        int number = 0;
        const auto text = d.description.toString().toStdString();
        if (clipdrag::parseVideoClip(text, from, number)) {
            if (onDropClip) onDropClip(from, number);
        } else if (clipdrag::parseVideoPad(text, from, number) && from == node && onDropPad) {
            onDropPad(number, juce::ModifierKeys::getCurrentModifiers().isAltDown());
        }
        repaint();
    }

    static bool accepts(const juce::String& path) {
        const auto ext = juce::File(path).getFileExtension().toLowerCase();
        return ext == ".mov" || ext == ".mp4" || ext == ".m4v" || ext == ".avi";
    }

private:
    static juce::String clock(double seconds) {
        const int tenths = (int) std::lround(std::max(0.0, seconds) * 10.0);
        const int total = tenths / 10;
        return juce::String(total / 60) + ":" + juce::String(total % 60).paddedLeft('0', 2)
               + "." + juce::String(tenths % 10);
    }

    padbar::Span span() const { return padbar::clipSpan(inSec_, outSec_, len_); }

    juce::String rangeText() const {
        if (fileName_.isEmpty()) return {};
        return clock(inSec_) + " - " + (outSec_ > inSec_ ? clock(outSec_) : juce::String("end"));
    }

    int row_;
    juce::String fileName_;
    juce::Image thumb_;
    bool active_ = false, outgoing_ = false, dragOver_ = false, dragged_ = false;
    bool badgeDrag_ = false, lifted_ = false;
    padbar::Grip grip_ = padbar::Grip::None;
    double pos_ = 0.0, len_ = 0.0, inSec_ = 0.0, outSec_ = 0.0;
    CellButton open_{IconGlyph::Open, {}}, play_{IconGlyph::Play, {}}, in_{IconGlyph::Play, "In"},
        out_{IconGlyph::Play, "Out"}, loop_{IconGlyph::Loop, {}};
};

class ClipGridBrick : public PolledBrick {
public:
    static constexpr int kCols = 4, kGap = 8, kParkTries = 200;
    static constexpr double kMaxScale = 2.5;
    static constexpr int kRows = VideoPadSource::kMaxClips / kCols;

    ClipGridBrick(EngineHost& host, std::string organism)
        : PolledBrick(host, std::move(organism), 2) {
        for (int i = 0; i < VideoPadSource::kMaxClips; ++i) {
            auto& cell = cells_[(size_t) i];
            cell = std::make_unique<ClipCell>(i);
            cell->node = name_;
            addAndMakeVisible(*cell);
            cell->onLaunch = [this, i] { launch(i); };
            cell->onPlayPause = [this, i] { playPause(i); };
            cell->onSetIn = [this, i] { stamp(i, "In"); };
            cell->onSetOut = [this, i] { stamp(i, "Out"); };
            cell->onLoop = [this, i] {
                host_.setParam(name_, "Loop" + suffix(i),
                               host_.liveParamValue(name_, "Loop" + suffix(i)) >= 0.5 ? 0.0 : 1.0);
            };
            cell->onOpen = [this, i] { choose(i); };
            cell->onDragIn = [this, i](double seconds) {
                host_.setParam(name_, "In" + suffix(i), std::round(seconds * 100.0) / 100.0);
            };
            cell->onDragOut = [this, i](double seconds) {
                host_.setParam(name_, "Out" + suffix(i), std::round(seconds * 100.0) / 100.0);
            };
            cell->onScrub = [this, i](double seconds) {
                parkWant_[(size_t) i] = -1.0;
                if (auto l = layer(i)) l->seekSeconds(seconds);
            };
            cell->onDrop = [this, i](const juce::File& f) {
                host_.setParamText(name_, "File" + suffix(i), f.getFullPathName().toStdString());
            };
            cell->onMenu = [this, i](juce::Point<int> at) { padMenu(i, at); };
            cell->onDropPad = [this, i](int from, bool move) { copyPad(from, i, move); };
            cell->onDropClip = [this, i](const std::string& track, int clipId) {
                takeClip(i, host_.clips().rangeOf(track, clipId));
            };
        }
        poll();
    }

    void reloadValues() override { poll(); }
    int preferredContentWidth() const override { return kCols * ClipCell::kWidth + (kCols - 1) * kGap; }
    int preferredContentHeight(int) const override {
        return kRows * ClipCell::kHeight + (kRows - 1) * kGap;
    }

    void resized() override {
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

private:
    static std::string suffix(int i) { return std::to_string(i + 1); }

    std::shared_ptr<VideoLayer> layer(int i) const {
        return VideoDeckPool::instance().peek(name_ + "/" + suffix(i));
    }

    void hold(int i, const juce::File& file, bool onStage) {
        auto& h = held_[(size_t) i];
        if (file == juce::File()) {
            h.reset();
            heldPath_[(size_t) i] = {};
            parkWant_[(size_t) i] = -1.0;
            return;
        }
        const auto path = file.getFullPathName();
        const double in = std::max(0.0, host_.liveParamValue(name_, "In" + suffix(i)));
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

    void armPark(int i, double seconds) {
        parkIn_[(size_t) i] = seconds;
        parkWant_[(size_t) i] = seconds;
        parkTries_[(size_t) i] = 0;
    }

    void park(int i) {
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

    void launch(int i) {
        host_.setParam(name_, "Launch" + suffix(i), 1.0);
        release_[(size_t) i] = true;
    }

    void playPause(int i) {
        auto* clips = live<VideoPadSource>();
        auto l = layer(i);
        if (clips != nullptr && clips->clipState().active == i && l != nullptr)
            l->setPaused(!l->isPaused());
        else
            launch(i);
    }

    void stamp(int i, const char* which) {
        auto l = layer(i);
        if (l == nullptr) return;
        host_.setParam(name_, which + suffix(i), std::round(l->positionSeconds() * 100.0) / 100.0);
    }

    void padMenu(int i, juce::Point<int> at) {
        const bool loaded = !host_.liveParamText(name_, "File" + suffix(i)).empty();
        juce::PopupMenu m;
        m.addItem(1, tr("clip-grid.clear-pad", "Clear pad"), loaded);
        m.addItem(2, tr("clip-grid.loop", "Loop"), loaded, host_.liveParamValue(name_, "Loop" + suffix(i)) >= 0.5);
        m.addSeparator();
        m.addItem(3, tr("clip-grid.launch-control", "Launch control..."));
        m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({at.x, at.y, 1, 1}),
                        [this, i, at](int r) {
                            if (r == 1) clearPad(i);
                            else if (r == 2)
                                host_.setParam(name_, "Loop" + suffix(i),
                                               host_.liveParamValue(name_, "Loop" + suffix(i)) >= 0.5
                                                   ? 0.0 : 1.0);
                            else if (r == 3)
                                showAutomateMenu(host_, name_, "Launch" + suffix(i), at, {});
                        });
    }

    void clearPad(int i) {
        host_.beginTransaction();
        host_.setParamText(name_, "File" + suffix(i), {});
        host_.setParam(name_, "In" + suffix(i), 0.0);
        host_.setParam(name_, "Out" + suffix(i), 0.0);
        host_.setParam(name_, "Loop" + suffix(i), 1.0);
        host_.endTransaction();
    }

    void copyPad(int a, int b, bool move) {
        if (a == b || a < 0 || b < 0 || a >= VideoPadSource::kMaxClips
            || b >= VideoPadSource::kMaxClips)
            return;
        struct Pad { std::string file; double in, out, loop; };
        auto read = [this](int i) {
            return Pad{host_.liveParamText(name_, "File" + suffix(i)),
                       host_.liveParamValue(name_, "In" + suffix(i)),
                       host_.liveParamValue(name_, "Out" + suffix(i)),
                       host_.liveParamValue(name_, "Loop" + suffix(i))};
        };
        auto writeRange = [this](int i, const Pad& pad) {
            host_.setParam(name_, "In" + suffix(i), pad.in);
            host_.setParam(name_, "Out" + suffix(i), pad.out);
            host_.setParam(name_, "Loop" + suffix(i), pad.loop);
        };
        const Pad from = read(a), to = read(b);
        host_.beginTransaction();
        host_.setParamText(name_, "File" + suffix(b), from.file);
        if (move) host_.setParamText(name_, "File" + suffix(a), to.file);
        writeRange(b, from);
        if (move) writeRange(a, to);
        host_.endTransaction();
    }

    void choose(int i) {
        const auto cur = juce::String(host_.liveParamText(name_, "File" + suffix(i)));
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
                                  host_.setParamText(name_, "File" + suffix(i),
                                                     f.getFullPathName().toStdString());
                              });
    }

    juce::Image thumbnailOf(const VideoLayer::Frame& f) const {
        return imageOfFrame(f, (int) std::lround(ClipCell::kWidth * scale_),
                            (int) std::lround(ClipCell::kThumbH * scale_));
    }

    void takeClip(int i, const ClipEditor::MediaRange& r) {
        if (r.file.empty()) return;
        host_.setParamText(name_, "File" + suffix(i), r.file);
        host_.setParam(name_, "In" + suffix(i), std::round(r.inSeconds * 100.0) / 100.0);
        host_.setParam(name_, "Out" + suffix(i), std::round(r.outSeconds * 100.0) / 100.0);
        host_.setParam(name_, "Loop" + suffix(i), r.looped ? 1.0 : 0.0);
    }

    void poll() override {
        VideoPadSource::ClipState st;
        auto* clips = live<VideoPadSource>();
        if (clips != nullptr) st = clips->clipState();
        const int pads = clips != nullptr ? clips->clipCount() : VideoPadSource::kMaxClips;
        for (int i = 0; i < VideoPadSource::kMaxClips; ++i) {
            cells_[(size_t) i]->setVisible(i < pads);
            if (release_[(size_t) i]) {
                release_[(size_t) i] = false;
                host_.setParam(name_, "Launch" + suffix(i), 0.0);
            }
            const auto path = juce::String(host_.liveParamText(name_, "File" + suffix(i)));
            const auto file = VideoDeckPool::resolveTape(host_.documentPath(), path);
            hold(i, file, st.active == i || st.outgoing == i);
            auto l = layer(i);
            const double len = l != nullptr ? l->lengthSeconds() : 0.0;
            if (clips != nullptr) clips->noteClipLength(i, len);
            auto& cell = *cells_[(size_t) i];
            cell.setState(file == juce::File() ? juce::String() : file.getFileName(),
                          st.active == i, st.outgoing == i, l != nullptr && l->isPaused(),
                          l != nullptr ? l->positionSeconds() : 0.0, len,
                          host_.liveParamValue(name_, "In" + suffix(i)),
                          host_.liveParamValue(name_, "Out" + suffix(i)),
                          host_.liveParamValue(name_, "Loop" + suffix(i)) >= 0.5);
            auto frame = l != nullptr ? l->latestFrame() : nullptr;
            if (frame != shown_[(size_t) i]) {
                shown_[(size_t) i] = frame;
                cell.setThumbnail(frame != nullptr ? thumbnailOf(*frame) : juce::Image());
            }
        }
    }

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
