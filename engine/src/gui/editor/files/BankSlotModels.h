// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <utility>
#include <vector>

#include "gui/editor/Words.h"
#include "gui/editor/files/FilePick.h"
#include "gui/host/ModelHost.h"
#include "io/PatchDocument.h"

namespace hum::files {

struct BankRef {
    std::string ref;
    std::string name;
};

inline constexpr Words kBankMissing{"bank-slot.missing", "missing: "};
inline constexpr Words kBankMissingTip{"bank-slot.missing-tip",
                                       "Not found - pick it again with Browse: "};
inline constexpr Words kNoBank{"bank-slot.no-bank", "(no bank)"};
inline constexpr Words kNoScale{"scale-browser.no-scale", "(no scale)"};

class BankShelf {
public:
    virtual ~BankShelf() = default;
    virtual std::vector<BankRef> factory() const = 0;
    virtual std::vector<BankRef> entries(const std::string& className) const = 0;
    virtual std::string referenceFor(const std::string& path, const std::string& className) const = 0;
    virtual void remember(const std::string& ref, const std::string& className) = 0;
    virtual std::string patterns() const = 0;
    virtual std::vector<std::string> schemes() const = 0;
    virtual bool available(const std::string& ref, const std::string& className) const = 0;
    virtual std::string keyFor(const std::string& ref, const std::string& className) const = 0;
    virtual std::string kind() const = 0;
    virtual Words emptyName() const { return kNoBank; }
};


class BankSlotModel {
public:
    struct Shown {
        std::string text;
        std::string tooltip;
        bool missing = false;
        bool clearable = false;
    };

    BankSlotModel(ModelHost& host, BankShelf& shelf, std::string organism, std::string param,
                  std::pair<std::string, double> select = {})
        : host_(host), shelf_(shelf), organism_(std::move(organism)), param_(std::move(param)),
          select_(std::move(select)) {}

    const std::string& param() const { return param_; }
    std::string ref() const { return host_.liveParamText(organism_, param_); }
    bool loaded() const { return !ref().empty(); }

    std::string display() const {
        const auto current = ref();
        const auto factory = shelf_.factory();
        for (const auto& e : factory)
            if (current == e.ref || shelf_.keyFor(current, className()) == e.ref) return e.name;
        if (current.empty()) return factory.empty() ? say(host_, shelf_.emptyName()) : factory.front().name;
        auto leaf = current;
        for (const auto& scheme : shelf_.schemes())
            if (leaf.rfind(scheme, 0) == 0) leaf = leaf.substr(scheme.size());
        const auto folder = fileNameOf(parentOf(leaf));
        if (folder.empty() || folder == shelf_.kind() || folder == "banks") return stemOf(leaf);
        return folder + " / " + stemOf(leaf);
    }

    Shown shown() const {
        const auto current = ref();
        Shown out;
        out.text = display();
        out.tooltip = current;
        out.clearable = !current.empty();
        out.missing = out.clearable && !shelf_.available(current, className());
        if (!out.missing) return out;
        out.text = say(host_, kBankMissing) + out.text;
        out.tooltip = say(host_, kBankMissingTip) + current;
        return out;
    }

    bool step(int direction) {
        const auto all = shelf_.entries(className());
        if (all.empty()) return false;
        const int at = indexOf(all);
        const int size = (int) all.size();
        const int next = at < 0 ? (direction > 0 ? 0 : size - 1)
                                : ((at + direction) % size + size) % size;
        load(all[(size_t) next].ref);
        return true;
    }

    void take(const std::string& bankRef) { load(bankRef); }
    void clear() { host_.setParamText(organism_, param_, ""); }

    FilePick request() const {
        FilePick pick;
        pick.title = "Open a bank";
        pick.patterns = shelf_.patterns();
        return pick;
    }

    bool chosen(const std::vector<std::string>& paths) {
        if (paths.empty()) return false;
        const auto cls = className();
        const auto bankRef = shelf_.referenceFor(paths.front(), cls);
        shelf_.remember(bankRef, cls);
        load(bankRef);
        return true;
    }

    std::string className() const {
        const auto* cm = host_.model().byName(organism_);
        return cm != nullptr ? cm->displayClass : std::string{};
    }

private:
    void load(const std::string& bankRef) {
        host_.setParamText(organism_, param_, bankRef);
        if (!select_.first.empty()) host_.editParam(organism_, select_.first, select_.second);
    }

    int indexOf(const std::vector<BankRef>& all) const {
        const auto current = ref();
        if (current.empty()) return shelf_.factory().empty() ? -1 : 0;
        const auto cls = className();
        const auto key = shelf_.keyFor(current, cls);
        for (int i = 0; i < (int) all.size(); ++i)
            if (shelf_.keyFor(all[(size_t) i].ref, cls) == key) return i;
        return -1;
    }

    ModelHost& host_;
    BankShelf& shelf_;
    std::string organism_, param_;
    std::pair<std::string, double> select_;
};

}
