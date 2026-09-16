// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/patcher/ModernPicker.h"

#include <cmath>

namespace hum {

ModernPicker::ModernPicker() {
    groups_ = classPickerGroups();

    search_.onKey = [this](const juce::KeyPress& k) { return handleKey(k); };
    search_.setFont(juce::FontOptions(14.0f));
    search_.setColour(juce::TextEditor::backgroundColourId, Palette::panelLight);
    search_.setColour(juce::TextEditor::textColourId, Palette::text);
    search_.setColour(juce::TextEditor::outlineColourId, Palette::border);
    search_.setColour(juce::TextEditor::focusedOutlineColourId, Palette::accentDim);
    search_.onTextChange = [this] { rebuild(); };
    search_.onEscapeKey = [this] { dismiss(); };
    search_.onReturnKey = [this] { activate(selected_); };
    addAndMakeVisible(search_);

    grid_.owner = this;
    view_.setViewedComponent(&grid_, false);
    view_.setScrollBarsShown(true, false);
    addAndMakeVisible(view_);

    setWantsKeyboardFocus(true);
    grid_.setWantsKeyboardFocus(false);
    setSize(560, 430);
    rebuild();
    startTimerHz(30);
}

bool ModernPicker::keyPressed(const juce::KeyPress& k) {
    if (k.getKeyCode() == juce::KeyPress::escapeKey) { dismiss(); return true; }
    if (k.getKeyCode() == juce::KeyPress::returnKey) { activate(selected_); return true; }
    if (handleKey(k)) return true;
    search_.grabKeyboardFocus();
    return search_.keyPressed(k);
}

float ModernPicker::marqueeForTest(int ticks) {
    selectedSinceMs_ = 0.0;
    for (int i = 0; i < ticks; ++i) timerCallback();
    return marqueeOffset_;
}

void ModernPicker::show(juce::Rectangle<int> screenAnchor, std::function<void(const std::string&)> onPick) {
    auto content = std::make_unique<ModernPicker>();
    content->onPick = std::move(onPick);
    juce::CallOutBox::launchAsynchronously(std::move(content), screenAnchor, nullptr);
}

void ModernPicker::resized() {
    auto r = getLocalBounds().reduced(10);
    crumbArea_ = r.removeFromTop(24);
    r.removeFromTop(6);
    search_.setBounds(r.removeFromTop(28));
    r.removeFromTop(6);
    view_.setBounds(r);
    layoutCards();
}

void ModernPicker::paint(juce::Graphics& g) {
    g.fillAll(Palette::background);
    crumbRects_.clear();
    const auto trail = picker::breadcrumb(path_);
    int x = crumbArea_.getX();
    const juce::Font pillFont = juce::FontOptions(12.0f);
    g.setFont(pillFont);
    for (size_t i = 0; i < trail.size(); ++i) {
        const auto label = juce::String::fromUTF8(trail[i].c_str());
        const int tw = (int) std::ceil(juce::GlyphArrangement::getStringWidth(pillFont, label));
        const juce::Rectangle<int> pill(x, crumbArea_.getY(), tw + 16, crumbArea_.getHeight());
        const bool last = i + 1 == trail.size();
        g.setColour(last ? Palette::accent.withAlpha(alpha::scrim) : Palette::panelLight);
        g.fillRoundedRectangle(pill.toFloat(), 10.0f);
        g.setColour(last ? Palette::text : Palette::textDim);
        g.drawText(label, pill, juce::Justification::centred);
        crumbRects_.push_back(pill);
        x = pill.getRight() + 4;
        if (!last) {
            g.setColour(Palette::textDim);
            g.drawText(juce::String::fromUTF8("\xe2\x80\xba"), x, crumbArea_.getY(), 10,
                       crumbArea_.getHeight(), juce::Justification::centred);
            x += 14;
        }
    }
}

void ModernPicker::mouseDown(const juce::MouseEvent& e) {
    const int i = crumbAt(e.getPosition());
    if (i >= 0) navigateTo(picker::Path(path_.begin(), path_.begin() + (long) i));
}

void ModernPicker::mouseMove(const juce::MouseEvent& e) {
    setMouseCursor(crumbAt(e.getPosition()) >= 0 ? juce::MouseCursor::PointingHandCursor
                                                 : juce::MouseCursor::NormalCursor);
}

void ModernPicker::mouseExit(const juce::MouseEvent&) {
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void ModernPicker::parentHierarchyChanged() {
    juce::MessageManager::callAsync([safe = juce::Component::SafePointer<ModernPicker>(this)] {
        if (safe != nullptr) safe->search_.grabKeyboardFocus();
    });
}

bool ModernPicker::hoverHand(juce::Component& c, juce::Point<int> p) {
    const auto now = juce::Time::getCurrentTime();
    const juce::MouseEvent e(juce::Desktop::getInstance().getMainMouseSource(), p.toFloat(),
                             juce::ModifierKeys(), 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, &c, &c, now,
                             p.toFloat(), now, 0, false);
    c.mouseMove(e);
    return c.getMouseCursor() == juce::MouseCursor::PointingHandCursor;
}

int ModernPicker::cellAt(juce::Point<int> p) const {
    for (size_t i = 0; i < cells_.size(); ++i)
        if (cells_[i].bounds.contains(p)) return (int) i;
    return -1;
}

int ModernPicker::crumbAt(juce::Point<int> p) const {
    for (size_t i = 0; i < crumbRects_.size(); ++i)
        if (crumbRects_[i].contains(p)) return (int) i;
    return -1;
}

std::string ModernPicker::pathKey(const picker::Path& p) {
    std::string key;
    for (const auto& s : p) key += s + "/";
    return key;
}

void ModernPicker::select(int i) {
    if (i == selected_) return;
    selected_ = i;
    selectedSinceMs_ = juce::Time::getMillisecondCounterHiRes();
    marqueeOffset_ = 0.0f;
    overflowPx_ = 0.0f;
}

void ModernPicker::timerCallback() {
    if (overflowPx_ <= 0.0f || selected_ < 0 || selected_ >= (int) cells_.size()) return;
    if (juce::Time::getMillisecondCounterHiRes() - selectedSinceMs_ < kMarqueeDelayMs) return;
    marqueeOffset_ += kMarqueeStep;
    const float wrap = overflowPx_ + kMarqueeRest;
    if (marqueeOffset_ > wrap) marqueeOffset_ = -kMarqueeRest;
    grid_.repaint(cells_[(size_t) selected_].bounds);
}

void ModernPicker::rebuild() {
    cells_.clear();
    const auto query = search_.getText().toStdString();
    if (query.empty()) {
        const auto level = picker::childrenAt(groups_, path_);
        for (const auto& f : level.folders) {
            Cell c;
            c.folder = true;
            c.f = f;
            cells_.push_back(std::move(c));
        }
        for (const auto& l : level.leaves) {
            Cell c;
            c.l = l;
            cells_.push_back(std::move(c));
        }
    } else {
        for (auto& l : picker::searchScoped(groups_, path_, query, [](const std::string& cls) {
                 return organismBlurb(cls).toStdString();
             })) {
            Cell c;
            c.l = std::move(l);
            cells_.push_back(std::move(c));
        }
    }
    int restored = 0;
    if (query.empty())
        if (const auto it = remembered_.find(pathKey(path_)); it != remembered_.end()) restored = it->second;
    selected_ = -1;
    select(cells_.empty() ? -1 : juce::jlimit(0, (int) cells_.size() - 1, restored));
    updateSearchPlaceholder();
    layoutCards();
    grid_.repaint();
    repaint();
}

void ModernPicker::layoutCards() {
    const int w = view_.getWidth() - view_.getScrollBarThickness() - 2;
    if (w <= 0) return;
    const int cardW = (w - (kCols - 1) * kGap) / kCols;
    int x = 0, y = 0, col = 0;
    for (auto& c : cells_) {
        c.bounds = {x, y, cardW, kCardH};
        if (++col == kCols) {
            col = 0;
            x = 0;
            y += kCardH + kGap;
        } else {
            x += cardW + kGap;
        }
    }
    const int rows = ((int) cells_.size() + kCols - 1) / kCols;
    grid_.setSize(w, juce::jmax(view_.getHeight(), rows * (kCardH + kGap)));
}

void ModernPicker::paintCard(juce::Graphics& g, size_t i) {
    const auto& c = cells_[i];
    const auto r = c.bounds.toFloat();
    g.setColour(Palette::panel);
    g.fillRoundedRectangle(r, 6.0f);
    g.setColour((int) i == selected_ ? Palette::accent : Palette::border);
    g.drawRoundedRectangle(r.reduced(0.5f), 6.0f, (int) i == selected_ ? 1.6f : 1.0f);

    auto inner = c.bounds.reduced(8, 5);
    const auto tile = inner.removeFromLeft(inner.getHeight());
    inner.removeFromLeft(10);
    if (c.folder) {
        g.setColour(Palette::panelLight);
        const auto fr = tile.toFloat().reduced(4.0f);
        g.fillRoundedRectangle(fr.withTrimmedTop(fr.getHeight() * 0.22f), 3.0f);
        g.fillRoundedRectangle(fr.withWidth(fr.getWidth() * 0.45f)
                                   .withHeight(fr.getHeight() * 0.34f), 3.0f);
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
        const auto count = juce::String(c.f.classCount)
                           + (c.f.classCount == 1 ? tr("modern-picker.organism", " organism")
                                                  : tr("modern-picker.organisms", " organisms"));
        g.drawText(juce::String::fromUTF8(c.f.label.c_str()), inner, juce::Justification::centredLeft);
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(12.0f));
        g.drawText(count, inner, juce::Justification::centredRight);
        return;
    }
    paintTile(g, c.l, tile);
    const auto tag = juce::String::fromUTF8(c.l.tag.c_str());
    const juce::Font tagFont(juce::FontOptions(11.0f));
    const int tagW = juce::jmin(inner.getWidth() / 3,
                                (int) std::ceil(juce::GlyphArrangement::getStringWidth(tagFont, tag)) + 4);
    auto tagArea = inner.removeFromRight(tagW);
    inner.removeFromRight(8);
    g.setColour(Palette::text);
    g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    g.drawText(juce::String::fromUTF8(c.l.display.c_str()),
               inner.removeFromTop(inner.getHeight() / 2), juce::Justification::bottomLeft);
    g.setColour(Palette::textDim);
    const juce::Font descFont(juce::FontOptions(11.5f));
    g.setFont(descFont);
    const auto desc = descriptionFor(c.l.cls);
    const float descW = juce::GlyphArrangement::getStringWidth(descFont, desc);
    const bool overflows = descW > (float) inner.getWidth();
    if ((int) i == selected_) overflowPx_ = overflows ? descW - (float) inner.getWidth() : 0.0f;
    if ((int) i == selected_ && overflows) {
        g.saveState();
        g.reduceClipRegion(inner);
        g.drawText(desc, inner.translated(-(int) std::max(0.0f, marqueeOffset_), 0).withWidth((int) descW + 2),
                   juce::Justification::topLeft, false);
        g.restoreState();
    } else {
        g.drawText(desc, inner, juce::Justification::topLeft, true);
    }
    g.setFont(tagFont);
    g.drawText(tag, tagArea, juce::Justification::centredRight, true);
}

void ModernPicker::paintTile(juce::Graphics& g, const picker::Leaf& leaf, juce::Rectangle<int> tile) {
    const bool plugin = isPluginKind(parseClassString(leaf.cls).kind);
    if (plugin) {
        const int tint = picker::tintIndex(leaf.cls, 8);
        g.setColour(Palette::accent.withRotatedHue(((float) tint - 3.5f) * 0.03f).withAlpha(alpha::scrim));
        g.fillRoundedRectangle(tile.toFloat().reduced(2.0f), 5.0f);
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
        g.drawText(juce::String::fromUTF8(picker::monogram(leaf.display).c_str()), tile,
                   juce::Justification::centred);
        return;
    }
    const Family family = familyOf(leaf.cls);
    g.saveState();
    juce::Path clip;
    clip.addRoundedRectangle(tile.toFloat().reduced(1.0f), 5.0f);
    g.reduceClipRegion(clip);
    collar::paintSoil(g, tile.toFloat().reduced(1.0f), family);
    g.restoreState();
    g.setImageResamplingQuality(juce::Graphics::lowResamplingQuality);
    g.drawImage(collar::grownMark(collar::speciesNameFor(leaf.display), family),
                tile.toFloat().reduced(4.0f));
}

void ModernPicker::updateSearchPlaceholder() {
    const auto scope = path_.empty()
        ? juce::String("organisms")
        : juce::String::fromUTF8(path_.back().c_str());
    search_.setTextToShowWhenEmpty("Search " + (path_.empty() ? scope : "in " + scope)
                                       + juce::String::fromUTF8("\xe2\x80\xa6"),
                                   Palette::textDim);
    search_.repaint();
}

void ModernPicker::navigateTo(picker::Path p) {
    if (search_.isEmpty() && selected_ >= 0) remembered_[pathKey(path_)] = selected_;
    path_ = std::move(p);
    search_.setText({}, juce::dontSendNotification);
    rebuild();
    view_.setViewPosition(0, 0);
    showSelected();
}

void ModernPicker::activate(int i) {
    if (i < 0 || i >= (int) cells_.size()) return;
    const auto& c = cells_[(size_t) i];
    if (c.folder) {
        if (search_.isEmpty()) remembered_[pathKey(path_)] = i;
        auto p = path_;
        p.push_back(c.f.label);
        navigateTo(std::move(p));
        return;
    }
    const auto cls = c.l.cls;
    dismiss();
    if (onPick) onPick(cls);
}

bool ModernPicker::handleKey(const juce::KeyPress& k) {
    const int n = (int) cells_.size();
    const int code = k.getKeyCode();
    if (code == juce::KeyPress::backspaceKey && search_.isEmpty()) {
        if (!path_.empty()) navigateTo(picker::parentOf(path_));
        return true;
    }
    if (n == 0) return false;
    int next = selected_;
    if (code == juce::KeyPress::downKey) next = juce::jmin(n - 1, selected_ + kCols);
    else if (code == juce::KeyPress::upKey) next = juce::jmax(0, selected_ - kCols);
    else if (code == juce::KeyPress::pageDownKey) next = juce::jmin(n - 1, selected_ + 8);
    else if (code == juce::KeyPress::pageUpKey) next = juce::jmax(0, selected_ - 8);
    else return false;
    select(next);
    showSelected();
    grid_.repaint();
    return true;
}

void ModernPicker::showSelected() {
    if (selected_ < 0 || selected_ >= (int) cells_.size()) return;
    const auto b = cells_[(size_t) selected_].bounds;
    auto pos = view_.getViewPosition();
    if (b.getBottom() > pos.y + view_.getHeight()) pos.y = b.getBottom() - view_.getHeight();
    if (b.getY() < pos.y) pos.y = b.getY();
    view_.setViewPosition(pos);
}

void ModernPicker::dismiss() {
    if (auto* box = findParentComponentOfClass<juce::CallOutBox>()) box->dismiss();
}

}
