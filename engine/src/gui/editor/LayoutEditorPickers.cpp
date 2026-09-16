// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/LayoutEditor.h"
#include "gui/bricks/FileTransport.h"
#include "gui/bricks/RhythmicUnitPicker.h"
#include "gui/bricks/BankBrowser.h"
#include "gui/bricks/ScaleBrowser.h"
#include "gui/bricks/SoundFileSlot.h"
#include "gui/editor/AutomateMenu.h"
#include "gui/common/PerfLog.h"
#include "gui/editor/BrickBindings.h"
#include "gui/tracks/StripHover.h"
#include <cmath>
#include <cstdlib>
#include "core/packs/PackRegistry.h"
#include "gui/editor/Mappable.h"
#include "gui/common/Localisation.h"
#include "gui/style/OriginColours.h"
#include "hum/ParseInt.h"

namespace hum {

namespace {
class ListArrow : public juce::Button {
public:
    explicit ListArrow(bool forward) : juce::Button({}), fwd_(forward) {
        setRepeatSpeed(400, 140);
    }
    void paintButton(juce::Graphics& g, bool over, bool down) override {
        const auto r = getLocalBounds().toFloat();
        const float w = 6.0f, h = 8.0f;
        const float x = r.getCentreX() - w * 0.5f, y = r.getCentreY() - h * 0.5f;
        juce::Path p;
        if (fwd_) p.addTriangle(x, y, x, y + h, x + w, y + h * 0.5f);
        else      p.addTriangle(x + w, y, x + w, y + h, x, y + h * 0.5f);
        g.setColour(!isEnabled() ? Palette::border.brighter(0.08f)
                    : (down || over) ? Palette::text : Palette::textDim);
        g.fillPath(p);
    }

private:
    bool fwd_;
};
}

void LayoutEditor::buildRhythmicUnit(const LayoutSpec::Control& s, Control& c, const ValueSetup& v) {
    const std::string cn = v.cn;
        auto ru = std::make_unique<RhythmicUnitPicker>(host_, cn, s.param, s.param2);
        addAndMakeVisible(*ru);
        auto* w = c.adopt(std::move(ru));
        c.reloadBrick = [w] { w->refresh(); };
}

void LayoutEditor::buildSoundFile(const LayoutSpec::Control& s, Control& c, const ValueSetup& v) {
    const std::string pn = v.pn;
    const std::string cn = v.cn;
        auto sf = std::make_unique<SoundFileSlot>(
            host_, cn, pn, s.extraOr("filter"), s.extraOr("title"),
            s.extraOr("kind"), s.extraOr("pick") == "save");
        if (const int origin = parseBoundedInt(s.extraOr("origin"), 2); origin >= 0)
            sf->setSwatch(originColour(origin));
        addAndMakeVisible(*sf);
        auto* w = c.adopt(std::move(sf));
        c.reloadBrick = [w] { w->refresh(); };
}

void LayoutEditor::buildScaleFile(const LayoutSpec::Control& s, Control& c, const ValueSetup& v) {
    const std::string pn = v.pn;
    const std::string cn = v.cn;
        auto sf = std::make_unique<ScaleFileSlot>(host_, cn, pn, Bindings(spec_, s));
        addAndMakeVisible(*sf);
        auto* w = c.adopt(std::move(sf));
        c.reloadBrick = [w] { w->refresh(); };
}

void LayoutEditor::buildBankFile(const LayoutSpec::Control& s, Control& c, const ValueSetup& v) {
    const std::string pn = v.pn;
    const std::string cn = v.cn;
        auto bf = std::make_unique<BankFileSlot>(
            host_, cn, pn,
            banks::Slot{s.extraOr("kind", "Samples"), s.extraOr("filter"),
                        s.extraOr("factory")});
        addAndMakeVisible(*bf);
        auto* w = c.adopt(std::move(bf));
        c.reloadBrick = [w] { w->refresh(); };
}

void LayoutEditor::buildFileTransport(const LayoutSpec::Control& s, Control& c, const ValueSetup& v) {
    const std::string cn = v.cn;
        auto tr = std::make_unique<FileTransport>(host_, cn, Bindings(spec_, s));
        addAndMakeVisible(*tr);
        c.adopt(std::move(tr));
}

void LayoutEditor::buildCombo(const LayoutSpec::Control& s, Control& c, const ValueSetup& v) {
    const std::string pn = v.pn;
    const std::string cn = v.cn;
        auto combo = std::make_unique<juce::ComboBox>();
        if (const auto key = s.extraOr("reclass"); !key.empty()) {
            const std::string cls = className();
            const size_t d0 = cls.find_first_of("0123456789");
            const size_t d1 = cls.find_first_not_of("0123456789", d0);
            if (key == "inputs" && d0 == std::string::npos) return;
            const std::string current = key == "inputs"
                ? cls.substr(d0, d1 - d0) : cls.substr(0, 1);
            const bool bare = key == "mode"
                && PackRegistry::instance().classManifest("S" + cls) != nullptr
                && [&] {
                       for (const auto& o : s.options)
                           if (o.substr(0, 1) == current) return false;
                       return true;
                   }();
            for (size_t i = 0; i < s.options.size(); ++i) {
                combo->addItem(juce::String::fromUTF8(s.options[i].c_str()), (int) i + 1);
                const auto& opt = s.options[i];
                if (bare ? opt.substr(0, 1) == "M"
                         : (key == "inputs" ? opt == current
                                            : opt.substr(0, 1) == current))
                    combo->setSelectedId((int) i + 1, juce::dontSendNotification);
            }
            combo->setColour(juce::ComboBox::textColourId, Palette::text);
            auto* cb = combo.get();
            combo->onChange = [this, cb, cn, key] {
                const std::string opt = cb->getText().toStdString();
                const std::string cur = className();
                if (opt.empty() || cur.empty()) return;
                std::string next;
                if (key == "inputs") {
                    const size_t a = cur.find_first_of("0123456789");
                    const size_t b = cur.find_first_not_of("0123456789", a);
                    if (a == std::string::npos) return;
                    next = cur.substr(0, a) + opt
                           + (b == std::string::npos ? std::string() : cur.substr(b));
                } else {
                    next = opt.substr(0, 1) + cur.substr(1);
                    if (PackRegistry::instance().classManifest(next) == nullptr
                        && PackRegistry::instance().classManifest(opt.substr(0, 1) + cur)
                               != nullptr)
                        next = opt.substr(0, 1) + cur;
                }
                if (next == cur) return;
                host_.replaceOrganism(cn, next);
                host_.noteTopologyChanged();
            };
            addAndMakeVisible(*combo);
            c.combo = std::move(combo);
            return;
        }
        c.comboIdsAreValues = !s.extraOr("source").empty();
        if (c.comboIdsAreValues) {
            auto live = std::make_unique<LiveSourceCombo>();
            auto* lc = live.get();
            auto fill = [this, lc, pn, cn, src = s.extraOr("source")] {
                const int current = (int) modelValue(pn);
                lc->clear(juce::dontSendNotification);
                bool listed = false;
                for (const auto& item : host_.choiceItems(src, cn)) {
                    lc->addItem(juce::String::fromUTF8(item.second.c_str()),
                                item.first);
                    listed |= item.first == current;
                }
                if (!listed && current > 0)
                    lc->addItem(juce::String(current) + tr("layout-editor.unavailable", " (unavailable)"), current);
                lc->setSelectedId(current, juce::dontSendNotification);
            };
            fill();
            lc->refreshItems = std::move(fill);
            combo = std::move(live);
        } else {
            for (size_t i = 0; i < s.options.size(); ++i)
                combo->addItem(juce::String::fromUTF8(s.options[i].c_str()), (int) i + 1);
        }
        combo->setColour(juce::ComboBox::textColourId, Palette::text);
        const int off = c.comboIdsAreValues ? 0 : 1;
        combo->setSelectedId((int) modelValue(pn) + off, juce::dontSendNotification);
        auto* cb = combo.get();
        juce::StringArray tips;
        tips.addTokens(juce::String::fromUTF8(s.extraOr("tips").c_str()), "|", "");
        auto showTip = [cb, tips, off] {
            const int i = cb->getSelectedId() - off;
            cb->setTooltip(juce::isPositiveAndBelow(i, tips.size()) ? tips[i] : juce::String());
        };
        if (!tips.isEmpty()) showTip();
        combo->onChange = [this, cb, cn, pn, off, showTip] {
            host_.editParam(cn, pn, cb->getSelectedId() - off);
            showTip();
        };
        addAndMakeVisible(*combo);
        if (!s.extraOr("steppers").empty()) {
            auto make = [this, cb, cn, pn, off](bool fwd) {
                auto b = std::make_unique<ListArrow>(fwd);
                b->onClick = [this, cb, cn, pn, off, fwd] {
                    if (auto* live = dynamic_cast<LiveSourceCombo*>(cb))
                        if (live->refreshItems) live->refreshItems();
                    const int n = cb->getNumItems();
                    if (n <= 0) return;
                    const int cur = std::max(0, cb->getSelectedItemIndex());
                    const int next = (cur + (fwd ? 1 : -1) + n) % n;
                    cb->setSelectedItemIndex(next, juce::dontSendNotification);
                    host_.editParam(cn, pn, (double) (cb->getItemId(next) - off));
                };
                addAndMakeVisible(*b);
                return b;
            };
            c.stepPrev = make(false);
            c.stepNext = make(true);
        }
        c.combo = std::move(combo);
}

}
