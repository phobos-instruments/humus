// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/bricks/ClipCell.h"

#include <cmath>

namespace hum {

ClipCell::ClipCell(int row)
    : row_(row) {
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

void ClipCell::setState(const juce::String& fileName, bool active, bool outgoing, bool paused, double pos, double len, double in, double out, bool loop) {
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

void ClipCell::setThumbnail(juce::Image img) {
    thumb_ = std::move(img);
    repaint();
}

void ClipCell::resized() {
    auto row = juce::Rectangle<int>(0, kThumbH + 4, kWidth, 22);
    const int bw = 26;
    for (auto* b : {&open_, &play_, &in_, &out_, &loop_}) {
        b->setBounds(row.removeFromLeft(bw));
        row.removeFromLeft(2);
    }
}

void ClipCell::paint(juce::Graphics& g) {
    auto thumb = juce::Rectangle<int>(0, 0, kWidth, kThumbH);
    g.setColour(Palette::panel.darker(0.3f));
    g.fillRoundedRectangle(thumb.toFloat(), 4.0f);
    if (thumb_.isValid()) {
        g.saveState();
        g.reduceClipRegion(thumb);
        g.drawImage(thumb_, thumb.toFloat(), juce::RectanglePlacement::fillDestination);
        g.restoreState();
    } else if (fileName_.isEmpty()) {
        g.setColour(Palette::textDim.withAlpha(alpha::mid));
        g.setFont(juce::FontOptions(10.0f));
        g.drawText(tr("clip-grid.drop-a-clip", "drop a clip"), thumb, juce::Justification::centred, false);
    }
    if (fileName_.isNotEmpty()) {
        auto band = thumb.withTrimmedTop(kThumbH - padbar::kTapeTop - 13)
                        .withTrimmedBottom(padbar::kTapeTop);
        g.setColour(Palette::background.withAlpha(alpha::strong));
        g.fillRect(band);
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(10.0f));
        g.drawText(fileName_, band.reduced(4, 0), juce::Justification::centredLeft, true);
    }
    if (len_ > 0.0) {
        const auto clip = span();
        const auto tape = juce::Rectangle<float>(
            0.0f, (float) (kThumbH - padbar::kTapeTop), (float) kWidth, (float) padbar::kTapeH);
        g.setColour(Palette::background.withAlpha(alpha::heavy));
        g.fillRect(tape.withTop(tape.getY() - 2.0f).withBottom((float) kThumbH));
        const float a = (float) padbar::xOfTape(clip.lo, kWidth, len_);
        const float b = (float) padbar::xOfTape(clip.hi, kWidth, len_);
        g.setColour(Palette::accent.withAlpha(alpha::mid));
        g.fillRect(a, tape.getY(), std::max(1.5f, b - a), tape.getHeight());
        for (const float x : {a, b}) {
            g.setColour(grip_ != padbar::Grip::None ? Palette::accent : Palette::text);
            g.fillRect(juce::jlimit(0.0f, (float) kWidth - 2.0f, x - 1.0f),
                       tape.getY() - 2.0f, 2.0f, tape.getHeight() + 3.0f);
        }
        const auto bar = juce::Rectangle<float>(0.0f, (float) (kThumbH - padbar::kBarH),
                                                (float) kWidth, (float) padbar::kBarH);
        g.setColour(Palette::accent.withAlpha(alpha::dim));
        g.fillRect(bar);
        const float p = (float) juce::jlimit(0.0, 1.0, (pos_ - clip.lo) / clip.length())
                      * bar.getWidth();
        g.setColour(Palette::text);
        g.fillRect(juce::jlimit(0.0f, (float) kWidth - 3.0f, p - 1.5f), bar.getY() - 1.0f,
                   3.0f, bar.getHeight() + 1.0f);
    }
    const auto badge = juce::Rectangle<float>(4.0f, 4.0f, 18.0f, 14.0f);
    g.setColour(active_ ? Palette::accent : Palette::background.withAlpha(alpha::strong));
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

void ClipCell::mouseDown(const juce::MouseEvent& e) {
    if (e.mods.isPopupMenu() && onMenu) onMenu(e.getScreenPosition());
    dragged_ = false;
    badgeDrag_ = fileName_.isNotEmpty() && e.x < 26 && e.y < 22;
    lifted_ = false;
    grip_ = padbar::gripAt(e.x, e.y, kWidth, kThumbH, span(), len_);
    if (grip_ != padbar::Grip::None) repaint();
}

void ClipCell::mouseMove(const juce::MouseEvent& e) {
    setMouseCursor(padbar::gripAt(e.x, e.y, kWidth, kThumbH, span(), len_) == padbar::Grip::None
                       ? juce::MouseCursor::NormalCursor
                       : juce::MouseCursor::LeftRightResizeCursor);
}

void ClipCell::mouseDrag(const juce::MouseEvent& e) {
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

void ClipCell::mouseUp(const juce::MouseEvent& e) {
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

bool ClipCell::isInterestedInFileDrag(const juce::StringArray& files) {
    return files.size() == 1 && accepts(files[0]);
}

void ClipCell::fileDragEnter(const juce::StringArray&, int, int) {
    dragOver_ = true;
    repaint();
}

void ClipCell::fileDragExit(const juce::StringArray&) {
    dragOver_ = false;
    repaint();
}

void ClipCell::filesDropped(const juce::StringArray& files, int, int) {
    dragOver_ = false;
    if (files.size() == 1 && onDrop) onDrop(juce::File(files[0]));
    repaint();
}

bool ClipCell::isInterestedInDragSource(const SourceDetails& d) {
    std::string from;
    int number = 0;
    const auto text = d.description.toString().toStdString();
    if (clipdrag::parseVideoClip(text, from, number)) return true;
    return clipdrag::parseVideoPad(text, from, number) && from == node && number != row_;
}

void ClipCell::itemDragEnter(const SourceDetails&) {
    dragOver_ = true;
    repaint();
}

void ClipCell::itemDragExit(const SourceDetails&) {
    dragOver_ = false;
    repaint();
}

void ClipCell::itemDropped(const SourceDetails& d) {
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

bool ClipCell::accepts(const juce::String& path) {
    const auto ext = juce::File(path).getFileExtension().toLowerCase();
    return ext == ".mov" || ext == ".mp4" || ext == ".m4v" || ext == ".avi";
}

juce::String ClipCell::clock(double seconds) {
    const int tenths = (int) std::lround(std::max(0.0, seconds) * 10.0);
    const int total = tenths / 10;
    return juce::String(total / 60) + ":" + juce::String(total % 60).paddedLeft('0', 2)
           + "." + juce::String(tenths % 10);
}

juce::String ClipCell::rangeText() const {
    if (fileName_.isEmpty()) return {};
    return clock(inSec_) + " - " + (outSec_ > inSec_ ? clock(outSec_) : juce::String("end"));
}

}
