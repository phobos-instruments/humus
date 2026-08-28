#pragma once
#include <functional>
#include <memory>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/ClassString.h"
#include "gui/EngineHost.h"
#include "gui/IconButton.h"
#include "gui/LookAndFeel.h"
#include "gui/PresetActions.h"
#include "gui/PresetLibrary.h"

namespace hum {

class StepArrow : public juce::Button {
public:
    explicit StepArrow(bool forward) : juce::Button({}), fwd_(forward) {
        setRepeatSpeed(400, 120);
    }
    std::function<void(bool)> onHover;
    std::function<void(juce::Point<int>)> onRightClick;

    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu() && onRightClick) { onRightClick(e.getScreenPosition()); return; }
        juce::Button::mouseDown(e);
    }

    void mouseEnter(const juce::MouseEvent& e) override {
        juce::Button::mouseEnter(e);
        if (onHover) onHover(true);
    }
    void mouseExit(const juce::MouseEvent& e) override {
        juce::Button::mouseExit(e);
        if (onHover) onHover(false);
    }

    void paintButton(juce::Graphics& g, bool over, bool down) override {
        const auto r = getLocalBounds().toFloat();
        const float w = 7.0f, h = 9.0f;
        const float x = r.getCentreX() - w * 0.5f, y = r.getCentreY() - h * 0.5f;
        juce::Path p;
        if (fwd_) p.addTriangle(x, y, x, y + h, x + w, y + h * 0.5f);
        else      p.addTriangle(x + w, y, x + w, y + h, x, y + h * 0.5f);
        g.setColour(!isEnabled() ? Palette::border.brighter(0.08f)
                    : (down || over) ? Palette::text
                                     : Palette::textDim);
        g.fillPath(p);
    }

private:
    bool fwd_;
};

class PresetField : public juce::Label {
public:
    PresetField() {
        setEditable(false, true, false);
        setFont(juce::FontOptions(12.0f));
        setBorderSize({0, 4, 0, 4});
        setJustificationType(juce::Justification::centred);
        setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::Label::outlineWhenEditingColourId, Palette::accentDim);
    }

    std::function<void()> onBrowse;
    std::function<void(juce::Point<int>)> onActMenu;
    std::function<void(bool)> onHover;

    void setSlot(int n) { if (n != slot_) { slot_ = n; repaint(); } }
    void setDrifted(bool d) { if (d != drift_) { drift_ = d; repaint(); } }
    bool drifted() const { return drift_; }

    void mouseEnter(const juce::MouseEvent& e) override {
        juce::Label::mouseEnter(e);
        hover_ = true;
        if (onHover) onHover(true);
        repaint();
    }
    void mouseExit(const juce::MouseEvent& e) override {
        juce::Label::mouseExit(e);
        hover_ = false;
        if (onHover) onHover(false);
        repaint();
    }

    void mouseDown(const juce::MouseEvent& e) override {
        grabKeyboardFocus();
        if (e.mods.isPopupMenu()) {
            if (onActMenu) onActMenu(e.getScreenPosition());
            return;
        }
        if (onBrowse) onBrowse();
    }

    void paint(juce::Graphics& g) override {
        if (isBeingEdited()) return juce::Label::paint(g);
        auto r = getLocalBounds().reduced(4, 0);
        const auto font = getFont();
        const auto txt = getText();

        const int slotW = slot_ > 0 ? kInk : 0;
        const int textW = juce::GlyphArrangement::getStringWidthInt(font, txt);
        const int groupW = juce::jmin(r.getWidth(), slotW + textW);
        auto grp = r.withWidth(groupW).withX(r.getX() + (r.getWidth() - groupW) / 2);

        if (slot_ > 0) {
            auto gutter = grp.removeFromLeft(kInk);
            g.setColour(drift_ ? Palette::textDim : Palette::accentDim);
            g.setFont(juce::FontOptions(10.5f).withStyle("Bold"));
            g.drawText(juce::String(slot_), gutter.removeFromLeft(kSlot),
                       juce::Justification::centredRight, false);
            if (drift_) {
                const auto c = gutter.toFloat().getCentre();
                g.setColour(Palette::warnAmber());
                g.fillEllipse(c.x - 1.5f, c.y - 1.5f, 3.0f, 3.0f);
            }
        }

        g.setColour(findColour(juce::Label::textColourId));
        g.setFont(font);
        g.drawText(txt, grp, juce::Justification::centred, true);

        if (hover_ && groupW + 22 <= r.getWidth()) {
            juce::Path p;
            const auto c = r.removeFromRight(11).toFloat().getCentre();
            p.addTriangle(c.x - 3.5f, c.y - 1.8f, c.x + 3.5f, c.y - 1.8f, c.x, c.y + 2.2f);
            g.setColour(Palette::textDim);
            g.fillPath(p);
        }
    }

private:
    static constexpr int kSlot = 16;
    static constexpr int kInk  = 28;
    int slot_ = 0;
    bool drift_ = false;
    bool hover_ = false;
};

class PresetRail : public juce::Component {
public:
    PresetRail(EngineHost& host, std::string node)
        : host_(host), node_(std::move(node)) {
        addAndMakeVisible(recall_);
        addAndMakeVisible(store_);
        addAndMakeVisible(evolve_);
        addAndMakeVisible(more_);
        addAndMakeVisible(prev_);
        addAndMakeVisible(next_);
        addAndMakeVisible(field_);

        recall_.setTooltip(juce::String::fromUTF8(
            "Recall - put the stored settings back, losing your changes"));
        store_.setTooltip(juce::String::fromUTF8(
            "Store - keep the current settings in this preset "
            "(hold Alt to store a new one)"));
        evolve_.setTooltip(juce::String::fromUTF8(
            "Evolve - nudge the settings; press again to wander further "
            "(one undo step)"));
        more_.setTooltip("More preset actions");
        prev_.setTooltip("Previous preset");
        next_.setTooltip("Next preset");
        store_.setActiveColour(Palette::warnAmber());

        recall_.onClick = [this] {
            if (const auto r = current(); r.valid())
                presets::recallBracketed(host_, node_,
                                         [this, r] { host_.presets().recall(node_, r); });
            changed();
        };
        store_.onClick = [this] { store(juce::ModifierKeys::currentModifiers.isAltDown()); };
        evolve_.onClick = [this] { presets::evolve(host_, node_); changed(); };
        more_.onClick = [this] { actMenu(more_.getScreenBounds().getBottomLeft()); };
        prev_.onClick = [this] { step(-1); };
        next_.onClick = [this] { step(+1); };
        prev_.onRightClick = [this](juce::Point<int> at) {
            showAutomateMenu(host_, node_, kPresetPrevAction, at, nullptr, false);
        };
        next_.onRightClick = [this](juce::Point<int> at) {
            showAutomateMenu(host_, node_, kPresetNextAction, at, nullptr, false);
        };

        auto hover = [this](bool h) { groupHover_ = h; repaint(); };
        prev_.onHover = hover;
        next_.onHover = hover;
        field_.onHover = hover;
        field_.onBrowse  = [this] { browseMenu(); };
        field_.onActMenu = [this](juce::Point<int> at) { actMenu(at); };
        field_.onTextChange = [this] {
            if (const auto r = current(); r.valid())
                host_.presets().rename(node_, r, field_.getText().toStdString());
            changed();
        };
        refresh();
    }

    void refresh() {
        const auto* c = host_.model().byName(node_);
        const auto all = stack();
        const auto cur = current();
        int idx = -1;
        for (size_t i = 0; i < all.size(); ++i) if (all[i].ref == cur) idx = (int) i;
        const bool has = idx >= 0;
        const bool drift = has && c != nullptr && c->presetDirty;
        const int count = (int) all.size();

        const juce::String nm = has ? juce::String(all[(size_t) idx].name) : juce::String();

        field_.setSlot(has ? idx + 1 : 0);
        field_.setDrifted(drift);
        field_.setColour(juce::Label::textColourId,
                         !has  ? Palette::textDim
                         : drift ? Palette::text
                                 : Palette::accent);
        field_.setText(has ? (nm.isEmpty() ? juce::String("(Untitled)") : nm)
                           : juce::String("(no preset)"),
                       juce::dontSendNotification);
        field_.setTooltip(has
            ? juce::String::fromUTF8("Current preset - click to browse, "
                                     "double-click to rename")
            : juce::String("No preset stored - click to store one"));

        recall_.setEnabled(has);
        store_.setOn(drift);
        store_.setLead(!has);
        prev_.setEnabled(count > 1);
        next_.setEnabled(count > 1);
    }

    std::function<void()> onChanged;
    std::function<void()> onOpenBrowser;

    struct Geometry {
        juce::Rectangle<int> recall, store, prev, field, next, evolve, more;
        bool evolveShown = false;
    };
    Geometry geometry() const {
        return {recall_.getBounds(), store_.getBounds(), prev_.getBounds(),
                field_.getBounds(), next_.getBounds(), evolve_.getBounds(),
                more_.getBounds(), evolve_.isVisible()};
    }
    bool drewDirty() const { return field_.drifted(); }

    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        g.setColour(Palette::background);
        g.fillRect(r);
        g.setColour(Palette::border.withAlpha(0.55f));
        g.drawHorizontalLine(0, 0.0f, r.getWidth());
        if (groupHover_) {
            g.setColour(Palette::panelLight.withAlpha(0.35f));
            g.fillRoundedRectangle(group_.toFloat(), 4.0f);
            g.setColour(Palette::border);
            g.drawRoundedRectangle(group_.toFloat().reduced(0.5f), 4.0f, 1.0f);
        }
    }

    void resized() override {
        auto row = getLocalBounds().reduced(4, 1);
        const int gap = row.getWidth() < 210 ? 4 : 8;

        recall_.setBounds(row.removeFromLeft(kBtn));
        row.removeFromLeft(2);
        store_.setBounds(row.removeFromLeft(kBtn));
        row.removeFromLeft(gap);

        more_.setBounds(row.removeFromRight(kBtn));
        evolve_.setVisible(row.getWidth() - kBtn - 2 - gap - 32 >= 60);
        if (evolve_.isVisible()) {
            row.removeFromRight(2);
            evolve_.setBounds(row.removeFromRight(kBtn));
        }
        row.removeFromRight(gap);

        group_ = row;
        prev_.setBounds(row.removeFromLeft(kArrow));
        next_.setBounds(row.removeFromRight(kArrow));
        field_.setBounds(row);
    }

private:
    static constexpr int kBtn = 22;
    static constexpr int kArrow = 16;

    presets::Ref current() const { return host_.presets().current(node_); }
    std::vector<presets::Entry> stack() const {
        return presets::stack(host_.model().byName(node_));
    }

    void changed() {
        refresh();
        if (onChanged) onChanged();
    }

    void step(int dir) {
        presets::recallBracketed(host_, node_,
                                 [this, dir] { host_.presets().recallAdjacent(node_, dir); });
        changed();
    }

    void store(bool asNew) {
        const auto cur = current();
        const bool fresh = asNew || !cur.valid();
        host_.presets().store(node_, fresh ? presets::Ref{} : cur);
        changed();
        if (fresh) field_.showEditor();
    }

    void confirmClear(const presets::Ref& ref) {
        const juce::String nm = juce::String(ref.name);
        const juce::String what = nm.isEmpty()
            ? juce::String("this preset")
            : juce::String::fromUTF8("\xe2\x80\x9c") + nm + juce::String::fromUTF8("\xe2\x80\x9d");
        if (ref.source == presets::Source::Shipped) return;
        const bool mine = ref.source == presets::Source::Patch;
        juce::AlertWindow::showOkCancelBox(
            juce::MessageBoxIconType::QuestionIcon,
            "Delete preset",
            mine ? "Delete " + what + " from this patch?"
                 : "Delete " + what + " from your user presets?",
            "Delete", "Cancel", this,
            juce::ModalCallbackFunction::create(
                [&host = host_, node = node_, ref,
                 sp = juce::Component::SafePointer<PresetRail>(this)](int r) {
                    if (r != 1) return;
                    host.presets().clear(node, ref);
                    if (sp) sp->changed();
                }));
    }

    void browseMenu() {
        const auto all = stack();
        const auto cur = current();
        auto item = [&](juce::PopupMenu& into, size_t i) {
            const auto& e = all[i];
            into.addItem((int) i + 1, e.name.empty() ? juce::String("(Untitled)") : juce::String(e.name),
                         true, e.ref == cur);
        };
        auto sourceMenu = [&](juce::PopupMenu& into, presets::Source src) {
            bool any = false;
            for (size_t i = 0; i < all.size(); ++i)
                if (all[i].ref.source == src && all[i].group.empty()) { item(into, i); any = true; }
            std::vector<std::string> groups;
            for (const auto& e : all)
                if (e.ref.source == src && !e.group.empty()
                    && std::find(groups.begin(), groups.end(), e.group) == groups.end())
                    groups.push_back(e.group);
            for (const auto& g : groups) {
                juce::PopupMenu sub;
                for (size_t i = 0; i < all.size(); ++i)
                    if (all[i].ref.source == src && all[i].group == g) item(sub, i);
                into.addSubMenu(juce::String(g), sub);
                any = true;
            }
            return any;
        };
        juce::PopupMenu m;
        sourceMenu(m, presets::Source::Shipped);
        juce::PopupMenu lib, mine;
        if (sourceMenu(lib, presets::Source::Library)) m.addSubMenu("User Presets", lib);
        if (sourceMenu(mine, presets::Source::Patch)) m.addSubMenu("This patch", mine);
        m.addSeparator();
        if (cur.valid() && cur.source != presets::Source::Patch)
            m.addItem(kPin, juce::String::fromUTF8("Copy \xe2\x80\x9c") + juce::String(cur.name)
                                + juce::String::fromUTF8("\xe2\x80\x9d into this patch"));
        m.addItem(kSaveLib, juce::String::fromUTF8("Save as User Preset\xe2\x80\xa6"));
        m.addItem(kBrowser, juce::String::fromUTF8("Presets window\xe2\x80\xa6"));
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&field_),
                        [sp = juce::Component::SafePointer<PresetRail>(this), all](int r) {
            if (sp == nullptr || r == 0) return;
            if (r == kBrowser) { if (sp->onOpenBrowser) sp->onOpenBrowser(); return; }
            if (r == kPin) { sp->pinCurrent(); return; }
            if (r == kSaveLib) { sp->promptSaveToLibrary(); return; }
            if (r < 1 || r > (int) all.size()) return;
            const auto ref = all[(size_t) r - 1].ref;
            presets::recallBracketed(sp->host_, sp->node_,
                                     [&sp, ref] { sp->host_.presets().recall(sp->node_, ref); });
            sp->changed();
        });
    }

    void pinCurrent() {
        const auto all = stack();
        const auto* e = presets::find(all, current());
        if (e == nullptr) return;
        PresetModel pm;
        pm.name = e->name;
        pm.properties = e->properties;
        const int n = host_.presets().adopt(node_, std::move(pm));
        if (n > 0) host_.presets().setCurrent(node_, {presets::Source::Patch, n, e->name});
        changed();
    }

    void adoptFromLibrary(const std::vector<PresetDef>& lib, size_t i) {
        if (i < lib.size()) adoptDef(lib[i]);
    }

    void adoptDef(const PresetDef& def) {
        const auto* c = host_.model().byName(node_);
        if (c == nullptr) return;
        const int n = host_.presets().adopt(
            node_, presetlib::toModel(def, schemaFor(c->classRaw), 0));
        if (n <= 0) return;
        const presets::Ref ref{presets::Source::Patch, n, def.name};
        presets::recallBracketed(host_, node_,
                                 [this, ref] { host_.presets().recall(node_, ref); });
        changed();
    }

    void exportPreset() {
        const auto* c = host_.model().byName(node_);
        if (c == nullptr) return;
        juce::String nm = juce::String(c->displayClass);
        for (const auto& pm : c->presets)
            if (pm.number == c->currentPreset && !pm.name.empty())
                nm = juce::String(pm.name);
        chooser_ = std::make_unique<juce::FileChooser>(
            "Export preset",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                .getChildFile(juce::File::createLegalFileName(nm) + ".humpreset"),
            "*.humpreset");
        chooser_->launchAsync(
            juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                | juce::FileBrowserComponent::warnAboutOverwriting,
            [sp = juce::Component::SafePointer<PresetRail>(this)](const juce::FileChooser& fc) {
                auto f = fc.getResult();
                if (f == juce::File() || sp == nullptr) return;
                if (!f.hasFileExtension("humpreset")) f = f.withFileExtension("humpreset");
                if (const auto* c = sp->host_.model().byName(sp->node_))
                    f.replaceWithText(juce::JSON::toString(presetlib::presetFileVar(
                        c->classRaw,
                        presetlib::capture(
                            *c, f.getFileNameWithoutExtension().toStdString()))));
            });
    }

    void importPreset() {
        chooser_ = std::make_unique<juce::FileChooser>(
            "Import preset",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
            "*.humpreset");
        chooser_->launchAsync(
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [sp = juce::Component::SafePointer<PresetRail>(this)](const juce::FileChooser& fc) {
                const auto f = fc.getResult();
                if (f == juce::File() || sp == nullptr) return;
                const auto* c = sp->host_.model().byName(sp->node_);
                if (c == nullptr) return;
                PresetDef def;
                const auto cls = presetlib::parsePresetFile(
                    juce::JSON::parse(f.loadFileAsString()), def);
                if (cls.empty() || parseClassString(cls).display != c->displayClass) {
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::MessageBoxIconType::WarningIcon, "Import preset",
                        cls.empty()
                            ? juce::String("This is not a Humus preset file.")
                            : "This preset belongs to "
                                  + juce::String(parseClassString(cls).display) + ", not "
                                  + juce::String(c->displayClass) + ".");
                    return;
                }
                sp->adoptDef(def);
            });
    }

    void promptSaveToLibrary() {
        const auto* c = host_.model().byName(node_);
        if (c == nullptr) return;
        juce::String initial = "My Preset";
        for (const auto& pm : c->presets)
            if (pm.number == c->currentPreset && !pm.name.empty())
                initial = juce::String(pm.name);
        auto* aw = new juce::AlertWindow(
            "Save to Library",
            "Name these settings. Library presets appear in every new "
                + juce::String(c->displayClass)
                + ". Saving to an existing name replaces it.",
            juce::MessageBoxIconType::NoIcon);
        aw->addTextEditor("name", initial);
        aw->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
        aw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
        aw->enterModalState(true, juce::ModalCallbackFunction::create(
            [aw, sp = juce::Component::SafePointer<PresetRail>(this)](int r) {
                const juce::String name = aw->getTextEditorContents("name").trim();
                if (r != 1 || name.isEmpty() || sp == nullptr) return;
                if (const auto* c = sp->host_.model().byName(sp->node_)) {
                    presetlib::save(c->classRaw, presetlib::capture(*c, name.toStdString()));
                }
            }), true);
    }

    void actMenu(juce::Point<int> at) {
        const auto cur = current();
        const bool has = cur.valid();
        const bool mine = has && cur.source == presets::Source::Patch;
        juce::PopupMenu m;
        if (!evolve_.isVisible()) { m.addItem(kEvolve, "Evolve"); m.addSeparator(); }
        m.addItem(kGenerate, juce::String::fromUTF8("Generate with AI\xe2\x80\xa6"));
        m.addSeparator();
        m.addItem(kRecall, "Recall", has);
        m.addItem(kStore, "Store", has);
        m.addItem(kStoreNew, "Store as new");
        m.addItem(kSaveLib, juce::String::fromUTF8("Save as User Preset\xe2\x80\xa6"));
        m.addItem(kRename, juce::String::fromUTF8("Rename\xe2\x80\xa6"), has);
        m.addItem(kClear, juce::String::fromUTF8("Delete\xe2\x80\xa6"),
                  has && cur.source != presets::Source::Shipped);
        m.addItem(kPin, "Pin to this patch", has && !mine);
        m.addSeparator();
        m.addItem(kCut, "Cut", mine);
        m.addItem(kCopy, "Copy", has);
        m.addItem(kPaste, "Paste", host_.presets().canPaste(node_));
        m.addSeparator();
        m.addItem(kExport, juce::String::fromUTF8("Export\xe2\x80\xa6"));
        m.addItem(kImport, juce::String::fromUTF8("Import\xe2\x80\xa6"));
        m.addSeparator();
        m.addItem(kBrowser, juce::String::fromUTF8("Presets window\xe2\x80\xa6"));
        m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({at.x, at.y, 1, 1}),
                        [sp = juce::Component::SafePointer<PresetRail>(this), cur](int r) {
            if (sp == nullptr || r == 0) return;
            auto& h = sp->host_;
            const auto& n = sp->node_;
            switch (r) {
                case kEvolve:   presets::evolve(h, n); sp->changed(); break;
                case kGenerate: presets::showGenie(h, n,
                                    [sp] { if (sp) sp->changed(); }); break;
                case kRecall:   presets::recallBracketed(h, n,
                                    [&h, &n, cur] { h.presets().recall(n, cur); });
                                sp->changed(); break;
                case kStore:    sp->store(false); break;
                case kStoreNew: sp->store(true); break;
                case kSaveLib:  sp->promptSaveToLibrary(); break;
                case kRename:   sp->field_.showEditor(); break;
                case kClear:    sp->confirmClear(cur); break;
                case kPin:      sp->pinCurrent(); break;
                case kCut:      h.presets().cut(n, cur); sp->changed(); break;
                case kCopy:     h.presets().copy(n, cur); sp->changed(); break;
                case kPaste:    h.presets().paste(n); sp->changed(); break;
                case kExport:   sp->exportPreset(); break;
                case kImport:   sp->importPreset(); break;
                case kBrowser:  if (sp->onOpenBrowser) sp->onOpenBrowser(); break;
                default: break;
            }
        });
    }

    enum : int {
        kEvolve = 10000, kGenerate, kRecall, kStore, kStoreNew, kSaveLib,
        kRename, kClear, kCut, kCopy, kPaste, kExport, kImport, kBrowser, kPin
    };

    EngineHost& host_;
    std::string node_;
    IconButton recall_{IconGlyph::Open, {}};
    IconButton store_{IconGlyph::Save, {}};
    IconButton evolve_{IconGlyph::Evolve, {}};
    IconButton more_{IconGlyph::Overflow, {}};
    StepArrow prev_{false}, next_{true};
    PresetField field_;
    std::unique_ptr<juce::FileChooser> chooser_;
    juce::Rectangle<int> group_;
    bool groupHover_ = false;
};

}
