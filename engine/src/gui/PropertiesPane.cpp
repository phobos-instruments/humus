#include "gui/PropertiesPane.h"

#include <algorithm>
#include <vector>

#include "gui/AppSettings.h"
#include "gui/ClassPickerMenu.h"
#include "gui/LookAndFeel.h"
#include "gui/MacCursors.h"
#include "gui/PickerLauncher.h"
#include "gui/QuickAddPalette.h"

namespace hum {

void PropertiesPane::Surface::paint(juce::Graphics& g) {
    g.fillAll(Palette::background);
    if (owner_.mode_ == LayoutMode::Blocks) {
        g.setColour(Palette::border.withAlpha(0.35f));
        const auto c = g.getClipBounds();
        const int x0 = 16 + juce::jmax(0, (c.getX() - 16) / 32) * 32;
        const int y0 = 16 + juce::jmax(0, (c.getY() - 16) / 32) * 32;
        for (int y = y0; y < c.getBottom(); y += 32)
            for (int x = x0; x < c.getRight(); x += 32)
                g.fillRect(x, y, 2, 2);
    }
    if (owner_.windows_.empty()) {
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(13.0f));
        g.drawText(juce::String::fromUTF8("Double-click an organism to edit its "
                                          "parameters  \xc2\xb7  right-click to add one"),
                   getLocalBounds().reduced(12), juce::Justification::centred, true);
    }
}

void PropertiesPane::Surface::mouseDown(const juce::MouseEvent& e) {
    if (!e.mods.isPopupMenu()) return;
    if (modernMenusEnabled()) {
        const auto sp = e.getScreenPosition();
        showCreatePicker({sp.x, sp.y, 1, 1}, [owner = &owner_](const std::string& cls) {
            if (owner->onAddOrganism) owner->onAddOrganism(cls);
        });
        return;
    }
    static constexpr int kSearch = 900001;
    auto ids = std::make_shared<std::vector<std::string>>();
    juce::PopupMenu menu;
    menu.addItem(kSearch, juce::String::fromUTF8("New Organism\xe2\x80\xa6"));
    menu.addSeparator();
    {
        juce::PopupMenu picker = classPickerMenu(0, *ids);
        for (juce::PopupMenu::MenuItemIterator it(picker); it.next();)
            menu.addItem(it.getItem());
    }
    const auto screen = e.getScreenPosition();
    menu.showMenuAsync(juce::PopupMenu::Options()
                           .withTargetScreenArea({screen.x, screen.y, 1, 1}),
                       [owner = &owner_, ids, screen](int r) {
        if (r == kSearch) {
            QuickAddPalette::show({screen.x, screen.y, 1, 1},
                                  [owner](const std::string& cls) {
                if (owner->onAddOrganism) owner->onAddOrganism(cls);
            });
            return;
        }
        if (r >= 1 && r <= (int) ids->size() && owner->onAddOrganism)
            owner->onAddOrganism((*ids)[(size_t) (r - 1)]);
    });
}

void PropertiesPane::buildModeStrip() {
    modeSwitch_.setTooltip(juce::String::fromUTF8(
        "Layout - Rack: stacked units, right edge snaps full/half."
        "  Free: place and resize boxes anywhere"));
    modeSwitch_.onChange = [this](bool second) {
        setLayoutMode(second ? LayoutMode::Blocks : LayoutMode::Rack);
    };
    addAndMakeVisible(modeSwitch_);
    mode_ = AppSettings::instance().getInt("properties.rackMode", 1) != 0
                ? LayoutMode::Rack : LayoutMode::Blocks;
    modeSwitch_.set(mode_ == LayoutMode::Blocks);
}

void PropertiesPane::setLayoutMode(LayoutMode m) {
    mode_ = m;
    placedFree_.clear();
    modeSwitch_.set(m == LayoutMode::Blocks);
    AppSettings::instance().set("properties.rackMode", m == LayoutMode::Rack ? 1 : 0);
    layoutBlocks();
}

void PropertiesPane::paint(juce::Graphics& g) {
    g.setColour(Palette::panelLight);
    g.fillRect(0, 0, getWidth(), kModeH);
    g.setColour(Palette::border);
    g.drawHorizontalLine(kModeH - 1, 0.0f, (float) getWidth());
}

void PropertiesPane::resized() {
    auto a = getLocalBounds();
    auto strip = a.removeFromTop(kModeH).reduced(2, 2);
    modeSwitch_.setBounds(strip.removeFromLeft(64));
    map_.setBounds(a.removeFromRight(kMapW));
    viewport_.setBounds(a);
    layoutBlocks();
}

float PropertiesPane::RackMap::scaleY() const {
    const int surfH = juce::jmax(1, owner_.surface_.getHeight());
    return (float) juce::jmax(1, getHeight() - 2 * kInset) / (float) surfH;
}

juce::Rectangle<float> PropertiesPane::RackMap::toMap(juce::Rectangle<int> r) const {
    const int surfW = juce::jmax(1, owner_.surface_.getWidth());
    const float sx = (float) juce::jmax(1, getWidth() - 2 * kInset) / (float) surfW;
    const float sy = scaleY();
    return {kInset + (float) r.getX() * sx, kInset + (float) r.getY() * sy,
            juce::jmax(3.0f, (float) r.getWidth() * sx),
            juce::jmax(2.0f, (float) r.getHeight() * sy)};
}

void PropertiesPane::RackMap::paint(juce::Graphics& g) {
    g.fillAll(Palette::panel);
    g.setColour(Palette::border);
    g.drawVerticalLine(0, 0.0f, (float) getHeight());
    for (auto& [n, w] : owner_.windows_) {
        const bool sel = n == owner_.selected_;
        g.setColour(sel ? Palette::accent : Palette::textDim.withAlpha(0.5f));
        g.fillRoundedRectangle(toMap(w->getBounds()), 1.5f);
    }
    const auto lens = toMap(owner_.viewport_.getViewArea());
    g.setColour(Palette::text.withAlpha(0.08f));
    g.fillRoundedRectangle(lens, 2.0f);
    g.setColour(Palette::accent.withAlpha(0.85f));
    g.drawRoundedRectangle(lens.reduced(0.5f), 2.0f, 1.2f);
}

void PropertiesPane::RackMap::seekTo(int mapY) {
    const auto view = owner_.viewport_.getViewArea();
    const int surfaceY = (int) ((float) (mapY - kInset) / scaleY());
    owner_.viewport_.seekTo(view.getX(),
                            juce::jmax(0, surfaceY - view.getHeight() / 2));
}

std::string PropertiesPane::RackMap::hitWindow(juce::Point<int> p) const {
    for (auto& [n, w] : owner_.windows_)
        if (toMap(w->getBounds()).expanded(1.5f).contains(p.toFloat())) return n;
    return {};
}

void PropertiesPane::RackMap::mouseDown(const juce::MouseEvent& e) {
    setMouseCursor(grabbingHandCursor());
    juce::Desktop::getInstance().getMainMouseSource().forceMouseCursorUpdate();
    pressedOn_ = hitWindow(e.getPosition());
    seekTo(e.y);
}

void PropertiesPane::RackMap::mouseDrag(const juce::MouseEvent& e) { seekTo(e.y); }

void PropertiesPane::RackMap::mouseUp(const juce::MouseEvent& e) {
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    juce::Desktop::getInstance().getMainMouseSource().forceMouseCursorUpdate();
    if (!e.mouseWasDraggedSinceMouseDown() && !pressedOn_.empty()
        && hitWindow(e.getPosition()) == pressedOn_) {
        owner_.setSelected(pressedOn_);
        if (owner_.onSelect) owner_.onSelect(pressedOn_);
    }
    pressedOn_.clear();
}

void PropertiesPane::RackMap::mouseWheelMove(const juce::MouseEvent&,
                                             const juce::MouseWheelDetails& w) {
    owner_.viewport_.smoothWheel(w.deltaY, w.isReversed);
}

void PropertiesPane::openFor(const std::string& name) {
    if (!host_.model().byName(name)) return;
    auto it = windows_.find(name);
    if (it != windows_.end()) {
        it->second->syncToModel();
        it->second->toFront(true);
        setSelected(name);
        layoutBlocks();
        return;
    }

    auto w = std::make_unique<ParameterWindow>(host_, name);
    w->onClose = [this](const std::string& nm) { closeFor(nm); };
    w->onOpenPluginUI = [this](const std::string& nm) { if (onOpenPluginUI) onOpenPluginUI(nm); };
    w->onMoved = [this](const std::string& nm, juce::Point<int>) { droppedAt(nm); };
    w->onSelect = [this](const std::string& nm) {
        setSelected(nm);
        if (onSelect) onSelect(nm);
    };
    w->onAutomationChanged = [this] { if (onAutomationChanged) onAutomationChanged(); };
    w->onContentResized = [this] { layoutBlocks(); };
    w->onResizeGesture = [this](const std::string& nm, int pw, int ph, bool done) {
        handleResizeGesture(nm, pw, ph, done);
    };
    w->onDragPreview = [this](const std::string& nm) { showDropPreview(nm); };
    host_.setEditorState(name, w->getPosition(), true);
    surface_.addAndMakeVisible(*w);
    windows_[name] = std::move(w);
    layoutBlocks();
    setSelected(name);
}

void PropertiesPane::closeFor(const std::string& name) {
    auto it = windows_.find(name);
    if (it == windows_.end()) return;
    const auto keep = mode_ == LayoutMode::Blocks ? it->second->getPosition()
                                                  : host_.editorPosition(name);
    host_.setEditorState(name, keep, false);
    windows_.erase(it);
    placedFree_.erase(name);
    if (selected_ == name) selected_.clear();
    if (mode_ == LayoutMode::Rack) layoutBlocks();
    else updateSurfaceSize();
}

void PropertiesPane::syncFromModel() {
    prune();
    for (auto& c : host_.model().organisms)
        if (host_.editorVisible(c.name) && windows_.find(c.name) == windows_.end())
            openFor(c.name);
    layoutBlocks();
}

void PropertiesPane::reload() {
    prune();
    for (auto& c : host_.model().organisms)
        if (host_.editorVisible(c.name) && windows_.find(c.name) == windows_.end())
            openFor(c.name);
    for (auto& [n, w] : windows_) w->reloadValues();
    layoutBlocks();
}

void PropertiesPane::reloadValuesFor(const std::string& name) {
    auto it = windows_.find(name);
    if (it != windows_.end()) it->second->reloadValues();
}

void PropertiesPane::refreshPresetState() {
    for (auto& [n, w] : windows_) w->refreshPresetState();
}

void PropertiesPane::refreshLiveValues() {
    for (auto& [n, w] : windows_) w->refreshLiveValues();
}

void PropertiesPane::refreshTextEdits() {
    const unsigned stamp = host_.textStamp();
    if (stamp == textStamp_) return;
    textStamp_ = stamp;
    for (auto& [n, w] : windows_) w->reloadTextValues();
}

void PropertiesPane::clearAll() {
    windows_.clear();
    placedFree_.clear();
    selected_.clear();
    updateSurfaceSize();
}

void PropertiesPane::prune() {
    bool removed = false;
    for (auto it = windows_.begin(); it != windows_.end();) {
        if (!host_.model().byName(it->first)) {
            if (selected_ == it->first) selected_.clear();
            placedFree_.erase(it->first);
            it = windows_.erase(it);
            removed = true;
        } else {
            ++it;
        }
    }
    if (!removed) return;
    if (mode_ == LayoutMode::Rack) layoutBlocks();
    else updateSurfaceSize();
}

void PropertiesPane::releaseEmbeddedEditors() {
    for (auto& [n, w] : windows_) w->releaseEmbedded();
}

void PropertiesPane::releaseEmbedded(const std::string& name) {
    if (auto it = windows_.find(name); it != windows_.end()) it->second->releaseEmbedded();
}

void PropertiesPane::releaseEmbeddedEditorsWhere(
    const std::function<bool(const std::string&)>& doomed) {
    for (auto& [n, w] : windows_)
        if (doomed(n)) w->releaseEmbedded();
}

void PropertiesPane::setSelected(const std::string& name) {
    selected_ = name;
    for (auto& [n, w] : windows_) w->setSelected(n == name);
    map_.repaint();
    auto it = windows_.find(name);
    if (it == windows_.end()) return;
    it->second->toFront(true);
    viewport_.cancelGlide();
    {
        const auto b = it->second->getBounds();
        const auto view = viewport_.getViewArea();
        if (b.getX() < view.getX())
            viewport_.setViewPosition(juce::jmax(0, b.getX() - 4),
                                      viewport_.getViewPositionY());
        else if (b.getRight() > view.getRight())
            viewport_.setViewPosition(juce::jmax(0, b.getRight() - view.getWidth() + 4),
                                      viewport_.getViewPositionY());
    }
    viewport_.glideTo([this, name] { return revealYFor(name); });
}

int PropertiesPane::revealYFor(const std::string& name) {
    const auto view = viewport_.getViewArea();
    auto it = windows_.find(name);
    if (it == windows_.end()) return view.getY();
    const auto b = it->second->getBounds();
    if (b.getY() < view.getY() || b.getHeight() + 8 > view.getHeight())
        return juce::jmax(0, b.getY() - 4);
    if (b.getBottom() > view.getBottom())
        return juce::jmax(0, b.getBottom() - view.getHeight() + 4);
    return view.getY();
}

}
