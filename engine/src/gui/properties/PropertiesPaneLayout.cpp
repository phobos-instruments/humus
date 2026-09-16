// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/style/Colours.h"
#include "gui/properties/PropertiesPane.h"

#include <algorithm>
#include <cmath>

#include "gui/style/LookAndFeel.h"

namespace hum {

namespace {
constexpr int kRackPadX = 0, kRackGap = 3;
}

void PropertiesPane::GhostOverlay::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat().reduced(1.5f);
    g.setColour(Palette::accent.withAlpha(alpha::mist));
    g.fillRoundedRectangle(r, 6.0f);
    juce::Path outline, dashed;
    outline.addRoundedRectangle(r, 6.0f);
    const float dash[] = {7.0f, 5.0f};
    juce::PathStrokeType(2.0f).createDashedStroke(dashed, outline, dash, 2);
    g.setColour(Palette::accent.withAlpha(alpha::nearOpaque));
    g.fillPath(dashed);
}

void PropertiesPane::showGhost(juce::Rectangle<int> r) {
    ghost_.setBounds(r);
    ghost_.setVisible(true);
    ghost_.toFront(false);
}

void PropertiesPane::hideGhost() { ghost_.setVisible(false); }

void PropertiesPane::updateSurfaceSize() {
    const int viewW = juce::jmax(0, viewport_.getMaximumVisibleWidth());
    const int viewH = juce::jmax(0, viewport_.getMaximumVisibleHeight());
    const bool racked = mode_ == LayoutMode::Rack;
    const int tail = racked ? 140 : 8;
    const int gutter = racked ? 0 : 8;
    int right = viewW, bottom = viewH;
    for (auto& [name, w] : windows_) {
        if (floating_.count(name)) continue;
        right  = juce::jmax(right,  w->getRight() + gutter);
        bottom = juce::jmax(bottom, w->getBottom() + tail);
    }
    surface_.setSize(right, bottom);
    surface_.repaint();
    map_.repaint();
}

void PropertiesPane::ensureOrder() {
    order_.erase(std::remove_if(order_.begin(), order_.end(),
                                [&](const std::string& n) { return windows_.find(n) == windows_.end(); }),
                 order_.end());
    std::vector<std::string> missing;
    for (auto& [n, w] : windows_)
        if (std::find(order_.begin(), order_.end(), n) == order_.end()) missing.push_back(n);
    std::sort(missing.begin(), missing.end(), [&](const std::string& a, const std::string& b) {
        const int ya = host_.editorPosition(a).y, yb = host_.editorPosition(b).y;
        return ya != yb ? ya < yb : a < b;
    });
    for (auto& n : missing) order_.push_back(n);
}

std::map<std::string, juce::Rectangle<int>> PropertiesPane::computeLayout(
        const std::vector<std::string>& ord) const {
    std::map<std::string, juce::Rectangle<int>> out;
    const int avail = juce::jmax(120, viewport_.getMaximumVisibleWidth());
    const int fullW = juce::jmax(120, avail - 2 * kRackPadX);
    const int halfW = (fullW - kRackGap) / 2;
    juce::ignoreUnused(halfW);
    int y = kRackGap;
    for (const auto& n : ord) {
        if (floating_.count(n)) continue;
        auto& a = *windows_.at(n);
        const int h = a.heightAtWidth(fullW);
        out[n] = {kRackPadX, y, fullW, h};
        y += h + kRackGap;
    }
    return out;
}

void PropertiesPane::layoutBlocks() {
    ensureOrder();
    auto& anim = juce::Desktop::getInstance().getAnimator();
    for (auto& [name, win] : windows_) anim.cancelAnimation(win.get(), false);

    if (mode_ == LayoutMode::Rack) {
        const auto view = viewport_.getViewArea();
        std::string anchor;
        int anchorOffset = 0;
        for (auto& n : order_) {
            auto* w = windows_.at(n).get();
            if (w->isBeingDragged()) continue;
            if (w->getBounds().getBottom() > view.getY()) {
                anchor = n;
                anchorOffset = w->getY() - view.getY();
                break;
            }
        }
        const auto rects = computeLayout(order_);
        const auto reach = view.expanded(0, view.getHeight() / 2);
        for (auto& n : order_) {
            if (floating_.count(n)) continue;
            auto* win = windows_[n].get();
            if (win->isBeingDragged()) continue;
            const auto r = rects.at(n);
            win->setLayoutDeferred(dragLayout_ && !r.intersects(reach));
            win->layoutForWidth(r.getWidth());
            win->setTopLeftPosition(r.getPosition());
            host_.setEditorState(n, r.getPosition(), true);
        }
        updateSurfaceSize();
        if (!anchor.empty())
            if (auto it = rects.find(anchor); it != rects.end())
                viewport_.setViewPosition(view.getX(),
                                          juce::jmax(0, it->second.getY() - anchorOffset));
        return;
    }

    for (auto& n : order_) {
        if (floating_.count(n)) continue;
        auto* win = windows_[n].get();
        if (win->isBeingDragged()) { placedFree_.insert(n); continue; }
        win->layoutNatural();
        juce::Point<int> p = win->getPosition();
        if (placedFree_.insert(n).second) {
            const auto stored = host_.editorPosition(n);
            p = nearestFreePos(stored.x >= 0 ? stored : juce::Point<int>{8, 8},
                               win->freeSize(), n);
        }
        p = snapFree(p);
        win->setTopLeftPosition(p);
        host_.setEditorState(n, p, true);
    }
    updateSurfaceSize();
}

juce::Point<int> PropertiesPane::snapFree(juce::Point<int> p) {
    constexpr int g = 16;
    return {juce::jmax(0, p.x / g * g), juce::jmax(0, p.y / g * g)};
}

juce::Point<int> PropertiesPane::nearestFreePos(juce::Point<int> want,
                                                juce::Point<int> size,
                                                const std::string& exclude) const {
    auto collides = [&](juce::Point<int> p) {
        const juce::Rectangle<int> r(p.x, p.y, size.x, size.y);
        for (auto& [n, w] : windows_) {
            if (n == exclude || !placedFree_.count(n)) continue;
            if (w->getBounds().intersects(r)) return true;
        }
        return false;
    };
    const auto base = snapFree(want);
    if (!collides(base)) return base;
    constexpr int g = 16;
    for (int ring = 1; ring <= 48; ++ring)
        for (int dy = -ring; dy <= ring; ++dy)
            for (int dx = -ring; dx <= ring; ++dx) {
                if (juce::jmax(std::abs(dx), std::abs(dy)) != ring) continue;
                const juce::Point<int> p{juce::jmax(0, base.x + dx * g),
                                         juce::jmax(0, base.y + dy * g)};
                if (!collides(p)) return p;
            }
    int bottom = 8;
    for (auto& [n, w] : windows_)
        if (placedFree_.count(n) && n != exclude) bottom = juce::jmax(bottom, w->getBottom());
    return snapFree({8, bottom + 8});
}

void PropertiesPane::resolveFreeOverlaps(const std::string& fixedName) {
    auto it = windows_.find(fixedName);
    if (it == windows_.end()) return;
    for (auto& [n, w] : windows_) {
        if (n == fixedName || !placedFree_.count(n)) continue;
        if (!it->second->getBounds().intersects(w->getBounds())) continue;
        const auto p = nearestFreePos(w->getPosition(),
                                      {w->getWidth(), w->getHeight()}, n);
        w->setTopLeftPosition(p);
        host_.setEditorState(n, p, true);
    }
    updateSurfaceSize();
}

std::vector<std::string> PropertiesPane::orderByPosition() const {
    auto ord = order_;
    std::stable_sort(ord.begin(), ord.end(), [&](const std::string& a, const std::string& b) {
        auto& wa = *windows_.at(a);
        auto& wb = *windows_.at(b);
        const int ya = wa.getBounds().getCentreY(), yb = wb.getBounds().getCentreY();
        if (std::abs(ya - yb) > juce::jmin(wa.getHeight(), wb.getHeight()) / 2)
            return ya < yb;
        return wa.getX() < wb.getX();
    });
    return ord;
}

void PropertiesPane::reflowFromDrop() {
    hideGhost();
    ensureOrder();
    order_ = orderByPosition();
    layoutBlocks();
}

void PropertiesPane::droppedAt(const std::string& name) {
    if (mode_ == LayoutMode::Rack) {
        hideGhost();
        layoutBlocks();
        return;
    }
    hideGhost();
    auto it = windows_.find(name);
    if (it == windows_.end()) return;
    const auto p = nearestFreePos(it->second->getPosition(), it->second->freeSize(), name);
    it->second->setTopLeftPosition(p);
    host_.setEditorState(name, p, true);
    updateSurfaceSize();
}

void PropertiesPane::showDropPreview(const std::string& name) {
    ensureOrder();
    if (mode_ == LayoutMode::Rack) return;
    if (mode_ == LayoutMode::Blocks) {
        if (auto it = windows_.find(name); it != windows_.end()) {
            const auto s = it->second->freeSize();
            const auto p = nearestFreePos(it->second->getPosition(), s, name);
            showGhost({p.x, p.y, s.x, s.y});
        }
        return;
    }
    const auto rects = computeLayout(orderByPosition());
    auto& anim = juce::Desktop::getInstance().getAnimator();
    for (auto& [n, r] : rects) {
        if (n == name) {
            showGhost(r);
            continue;
        }
        auto* win = windows_.at(n).get();
        if (win->getPosition() != r.getPosition() && !anim.isAnimating(win))
            anim.animateComponent(win, r.withSize(win->getWidth(), win->getHeight()),
                                  1.0f, 140, false, 1.0, 1.0);
    }
}

void PropertiesPane::handleResizeGesture(const std::string& name, int w, int h, bool done) {
    auto it = windows_.find(name);
    if (it == windows_.end()) return;
    auto* win = it->second.get();

    if (mode_ == LayoutMode::Rack) {
        hideGhost();
        return;
    }

    juce::ignoreUnused(h);
    const int cw = juce::jlimit((int) ParameterWindow::kMinW, win->maxUsefulWidth(), w);
    const int ch = win->heightAtWidth(cw);
    win->setUserSize(cw, ch);
    if (done) host_.setEditorSize(name, {cw, ch}, host_.editorHalf(name));
    layoutBlocks();
    resolveFreeOverlaps(name);
}

void PropertiesPane::setDragLayout(bool on) {
    if (dragLayout_ == on) return;
    dragLayout_ = on;
    if (!on)
        for (auto& [n, w] : windows_) w->setLayoutDeferred(false);
}

void PropertiesPane::flushVisibleLayouts() {
    if (!dragLayout_) return;
    const auto reach = viewport_.getViewArea().expanded(0, viewport_.getViewHeight() / 2);
    for (auto& [n, w] : windows_)
        if (w->getBounds().intersects(reach)) w->setLayoutDeferred(false);
}

void PropertiesPane::detach(const std::string& name) {
    auto it = windows_.find(name);
    if (it == windows_.end() || floating_.count(name)) return;
    auto* win = it->second.get();
    win->setFloating(true);
    win->layoutNatural();
    auto stored = host_.editorFloatBounds(name);
    auto fw = std::make_unique<FloatingBoxWindow>(
        juce::String(name), win, stored,
        [safe = juce::Component::SafePointer<PropertiesPane>(this), name] {
            juce::MessageManager::callAsync([safe, name] { if (safe != nullptr) safe->redock(name); });
        },
        [this, name](juce::Rectangle<int> b) { host_.setEditorFloating(name, true, b); });
    fw->onUnhandledKey = [this](const juce::KeyPress& k) { return onFloatKey && onFloatKey(k); };
    fw->onUnhandledKeyState = [this](bool down) { return onFloatKeyState && onFloatKeyState(down); };
    if (fw->getWidth() >= 160 && fw->getHeight() >= 160) host_.setEditorFloating(name, true, fw->getBounds());
    else host_.setEditorFloating(name, true, {});
    floating_[name] = std::move(fw);
    layoutBlocks();
}

void PropertiesPane::redock(const std::string& name) {
    auto fit = floating_.find(name);
    if (fit == floating_.end()) return;
    const auto last = fit->second->getBounds();
    fit->second->releaseContent();
    floating_.erase(fit);
    host_.setEditorFloating(name, false, last);
    if (auto it = windows_.find(name); it != windows_.end()) {
        it->second->setFloating(false);
        surface_.addAndMakeVisible(*it->second);
    }
    layoutBlocks();
}

void PropertiesPane::moveInOrder(const std::string& name, int delta) {
    ensureOrder();
    std::vector<std::string> racked;
    for (const auto& n : order_) if (!floating_.count(n)) racked.push_back(n);
    const auto it = std::find(racked.begin(), racked.end(), name);
    if (it == racked.end()) return;
    const int at = (int) std::distance(racked.begin(), it);
    const int to = juce::jlimit(0, (int) racked.size() - 1, at + delta);
    if (to == at) return;
    std::swap(racked[(size_t) at], racked[(size_t) to]);
    std::vector<std::string> next;
    size_t k = 0;
    for (const auto& n : order_) next.push_back(floating_.count(n) ? n : racked[k++]);
    order_ = std::move(next);
    const std::string other = racked[(size_t) at];
    std::map<std::string, juce::Point<int>> before;
    for (const auto& n : {name, other}) before[n] = windows_.at(n)->getPosition();
    layoutBlocks();
    auto& anim = juce::Desktop::getInstance().getAnimator();
    for (const auto& n : {name, other}) {
        if (!isShowing()) break;
        auto* win = windows_.at(n).get();
        const auto target = win->getBounds();
        win->setTopLeftPosition(before.at(n));
        anim.animateComponent(win, target, 1.0f, 160, false, 1.0, 1.0);
    }
    setSelected(name);
}

void PropertiesPane::autoArrangeFree() {
    if (mode_ != LayoutMode::Blocks) return;
    ensureOrder();
    const int avail = juce::jmax(200, viewport_.getMaximumVisibleWidth());
    constexpr int gap = 8;
    int x = gap, y = gap, rowH = 0;
    for (const auto& n : order_) {
        if (floating_.count(n)) continue;
        auto* win = windows_.at(n).get();
        win->layoutNatural();
        const auto s = win->freeSize();
        if (x > gap && x + s.x > avail) { x = gap; y += rowH + gap; rowH = 0; }
        win->setTopLeftPosition(x, y);
        placedFree_.insert(n);
        host_.setEditorState(n, {x, y}, true);
        x += s.x + gap;
        rowH = juce::jmax(rowH, s.y);
    }
    updateSurfaceSize();
}

void PropertiesPane::closeFloatingForTest(const std::string& name) {
    if (auto fit = floating_.find(name); fit != floating_.end()) fit->second->closeButtonPressed();
}

}
