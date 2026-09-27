// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/Faceplate.h"
#include "gui/editor/Words.h"

#include <algorithm>
#include <cstdlib>
#include <memory>

#include "gui/editor/ChoiceModel.h"
#include "gui/editor/LayoutModel.h"
#include "gui/editor/ControlModel.h"

namespace hum {

namespace {

constexpr Words kUnavailable{"layout-editor.unavailable", " (unavailable)"};

}

void Faceplate::buildReclassCombo(const LayoutSpec::Control& s, Control& c, const std::string& key) {
    const auto known = [this](const std::string& cls) { return owner_.knowsClass(cls); };
    const std::string cls = className();
    const size_t d0 = cls.find_first_of("0123456789");
    const size_t d1 = cls.find_first_not_of("0123456789", d0);
    const bool numbered = d0 != std::string::npos;
    const std::string current = key != "inputs" ? cls.substr(0, 1)
                              : numbered ? cls.substr(d0, d1 - d0) : std::string();
    const bool bare = key == "mode" && known("S" + cls) && [&] {
        for (const auto& o : s.options)
            if (o.substr(0, 1) == current) return false;
        return true;
    }();
    auto setup = choice::setupFor(s, name_);
    setup.art = art::forControl(s, spec_.dir);
    setup.hasSteppers = false;
    auto view = views_.combo(setup);
    std::vector<ComboItem> items;
    int selected = 0;
    for (size_t i = 0; i < s.options.size(); ++i) {
        items.push_back({(int) i + 1, s.options[i]});
        const auto& opt = s.options[i];
        if (bare ? opt.substr(0, 1) == "M" : (key == "inputs" ? opt == current : opt.substr(0, 1) == current))
            selected = (int) i + 1;
    }
    view->setItems(items);
    if (selected > 0) view->showSelectedId(selected);
    auto* vp = view.get();
    vp->onPick = [this, vp, key, known](int) {
        const std::string opt = vp->shownText();
        const std::string cur = className();
        if (opt.empty() || cur.empty()) return;
        std::string next;
        if (key == "inputs") {
            const size_t a = cur.find_first_of("0123456789");
            const size_t b = cur.find_first_not_of("0123456789", a);
            next = a == std::string::npos
                ? cur + opt
                : cur.substr(0, a) + opt + (b == std::string::npos ? std::string() : cur.substr(b));
            if (!known(next)) return;
        } else {
            next = opt.substr(0, 1) + cur.substr(1);
            if (!known(next) && known(opt.substr(0, 1) + cur)) next = opt.substr(0, 1) + cur;
        }
        if (next == cur) return;
        host_.replaceOrganism(name_, next);
        host_.noteTopologyChanged();
    };
    c.combo = std::move(view);
}

void Faceplate::buildCombo(const LayoutSpec::Control& s, Control& c) {
    if (const auto key = s.extraOr("reclass"); !key.empty()) {
        buildReclassCombo(s, c, key);
        return;
    }
    const std::string pn = s.param;
    auto setup = choice::setupFor(s, name_);
    setup.art = art::forControl(s, spec_.dir);
    c.comboIdsAreValues = setup.idsAreValues;
    auto view = views_.combo(setup);
    auto* vp = view.get();
    auto items = std::make_shared<std::vector<ComboItem>>();
    const int needs = std::max(0, std::atoi(s.extraOr("needs-choice", "0").c_str()));
    if (c.comboIdsAreValues) {
        auto fill = [this, vp, items, pn, needs, src = setup.source] {
            const int current = (int) control::paramValue(host_, name_, pn);
            items->clear();
            bool listed = false;
            for (const auto& item : host_.choiceItems(src, name_)) {
                items->push_back({choice::comboIdForSourceValue(item.first), item.second});
                listed |= item.first == current;
            }
            if (!listed && current > 0)
                items->push_back(
                    {current, std::to_string(current) + host_.translated(kUnavailable.key, kUnavailable.written)});
            vp->setItems(*items);
            vp->showSelectedId(choice::comboIdForSourceValue(current));
            if (needs > 0) {
                const bool offered = (int) items->size() >= needs;
                vp->setViewFade(layout::alphaFor(!offered), offered);
            }
        };
        fill();
        vp->onOpen = fill;
        c.refillCombo = fill;
    } else if (const auto offer = choice::offerOf(s); !offer.empty()) {
        auto fill = [this, vp, items, setup, offer, options = s.options] {
            const double value = control::paramValue(host_, setup.organism, setup.param);
            items->clear();
            for (const auto& o : choice::offered(options, offer, choice::indexForValue(setup, value)))
                items->push_back({o.index + 1, o.text});
            vp->setItems(*items);
            vp->showSelectedId(choice::comboIdForValue(setup, value));
        };
        fill();
        vp->onOpen = fill;
        c.refillCombo = fill;
    } else {
        for (size_t i = 0; i < s.options.size(); ++i) items->push_back({(int) i + 1, s.options[i]});
        vp->setItems(*items);
    }
    const int off = c.comboIdsAreValues ? 0 : 1;
    vp->showSelectedId(choice::comboIdForValue(setup, control::paramValue(host_, name_, pn)));
    auto showTip = [vp, setup, off] { vp->showTip(choice::tipForIndex(setup, vp->shownSelectedId() - off)); };
    if (!s.extraOr("tips").empty()) showTip();
    vp->onPick = [this, setup, showTip](int id) {
        host_.editParam(setup.organism, setup.param, choice::valueForComboId(setup, id));
        showTip();
        applyDims();
    };
    if (setup.hasSteppers)
        vp->onStep = [this, vp, items, pn, off, refill = c.refillCombo](bool forward) {
            if (refill) refill();
            const int n = (int) items->size();
            if (n <= 0) return;
            int cur = 0;
            for (int i = 0; i < n; ++i)
                if ((*items)[(size_t) i].id == vp->shownSelectedId()) cur = i;
            const int next = choice::wrapped(n, cur, forward);
            vp->showSelectedId((*items)[(size_t) next].id);
            host_.editParam(name_, pn, (double) ((*items)[(size_t) next].id - off));
            applyDims();
        };
    c.combo = std::move(view);
}

}
