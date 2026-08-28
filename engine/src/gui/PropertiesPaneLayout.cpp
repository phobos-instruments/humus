#include "gui/PropertiesPane.h"

#include <algorithm>
#include <cmath>

#include "gui/LookAndFeel.h"

namespace hum {

namespace {
constexpr int kRackPadX = 18, kRackGap = 14;
}

void PropertiesPane::GhostOverlay::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat().reduced(1.5f);
    g.setColour(Palette::accent.withAlpha(0.10f));
    g.fillRoundedRectangle(r, 6.0f);
    juce::Path outline, dashed;
    outline.addRoundedRectangle(r, 6.0f);
    const float dash[] = {7.0f, 5.0f};
    juce::PathStrokeType(2.0f).createDashedStroke(dashed, outline, dash, 2);
    g.setColour(Palette::accent.withAlpha(0.9f));
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
    const int tail = mode_ == LayoutMode::Rack ? 140 : 8;
    int right = viewW, bottom = viewH;
    for (auto& [name, w] : windows_) {
        right  = juce::jmax(right,  w->getRight() + 8);
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
    std::sort(missing.begin(), missing.end());
    for (auto& n : missing) order_.push_back(n);
}

std::map<std::string, juce::Rectangle<int>> PropertiesPane::computeLayout(
        const std::vector<std::string>& ord) const {
    std::map<std::string, juce::Rectangle<int>> out;
    const int avail = juce::jmax(120, viewport_.getMaximumVisibleWidth());
    const int fullW = juce::jmax(120, avail - 2 * kRackPadX);
    const int halfW = (fullW - kRackGap) / 2;
    int y = kRackGap;
    for (size_t i = 0; i < ord.size();) {
        auto& a = *windows_.at(ord[i]);
        if (a.halfRack() && i + 1 < ord.size() && windows_.at(ord[i + 1])->halfRack()) {
            auto& b = *windows_.at(ord[i + 1]);
            const int ha = a.heightAtWidth(halfW), hb = b.heightAtWidth(halfW);
            out[ord[i]]     = {kRackPadX, y, halfW, ha};
            out[ord[i + 1]] = {kRackPadX + halfW + kRackGap, y, halfW, hb};
            y += juce::jmax(ha, hb) + kRackGap;
            i += 2;
        } else {
            const int w = a.halfRack() ? halfW : fullW;
            const int h = a.heightAtWidth(w);
            out[ord[i]] = {kRackPadX, y, w, h};
            y += h + kRackGap;
            ++i;
        }
    }
    return out;
}

void PropertiesPane::layoutBlocks() {
    ensureOrder();
    juce::Desktop::getInstance().getAnimator().cancelAllAnimations(false);

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
        for (auto& n : order_) {
            auto* win = windows_[n].get();
            if (win->isBeingDragged()) continue;
            const auto r = rects.at(n);
            win->layoutForWidth(r.getWidth());
            win->setTopLeftPosition(r.getPosition());
            host_.setEditorState(n, host_.editorPosition(n), true);
        }
        updateSurfaceSize();
        if (!anchor.empty())
            if (auto it = rects.find(anchor); it != rects.end())
                viewport_.setViewPosition(view.getX(),
                                          juce::jmax(0, it->second.getY() - anchorOffset));
        return;
    }

    for (auto& n : order_) {
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
        reflowFromDrop();
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
        const int avail = juce::jmax(120, viewport_.getMaximumVisibleWidth());
        const int fullW = juce::jmax(120, avail - 2 * kRackPadX);
        const int halfW = (fullW - kRackGap) / 2;
        const bool half = w < (halfW + fullW) / 2;
        if (!done) {
            const int gw = half ? halfW : fullW;
            showGhost({win->getX(), win->getY(), gw, win->heightAtWidth(gw)});
            return;
        }
        hideGhost();
        if (half != win->halfRack()) {
            win->setHalfRack(half);
            host_.setEditorSize(name, host_.editorSize(name), half);
            layoutBlocks();
        }
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

}
