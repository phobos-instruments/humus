// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/help/HelpBrowser.h"
#include "gui/host/EngineHost.h"

namespace hum {

HelpBrowser::~HelpBrowser() = default;

HelpBrowser::HelpBrowser() {
    buildOutline();

    search_.setFont(juce::FontOptions(14.0f));
    search_.setTextToShowWhenEmpty(juce::String::fromUTF8("Search organisms\xe2\x80\xa6"),
                                   Palette::textDim);
    search_.setColour(juce::TextEditor::backgroundColourId, Palette::panelLight);
    search_.setColour(juce::TextEditor::textColourId, Palette::text);
    search_.setColour(juce::TextEditor::outlineColourId, Palette::border);
    search_.setColour(juce::TextEditor::focusedOutlineColourId, Palette::accentDim);
    search_.onTextChange = [this] { refilter(); };
    addAndMakeVisible(search_);

    list_.setModel(this);
    list_.setRowHeight(22);
    list_.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(list_);

    heading_.setFont(juce::Font(juce::FontOptions(14.0f).withStyle("Bold")));
    heading_.setColour(juce::Label::textColourId, Palette::accent);
    addAndMakeVisible(heading_);

    viewport_.setScrollBarsShown(true, false);
    addAndMakeVisible(viewport_);

    refilter();
    setSize(780, 500);
}

void HelpBrowser::showClass(const std::string& wanted) {
    const auto it = rowOf_.find(wanted);
    const std::string displayClass = it == rowOf_.end() ? wanted : it->second;
    search_.setText({}, juce::dontSendNotification);
    std::vector<int> ancestorHeaderIds;
    for (const auto& e : outline_) {
        if (e.kind == Row::Header) {
            ancestorHeaderIds.resize((size_t) e.level);
            ancestorHeaderIds.push_back(e.id);
        } else if (e.display == displayClass) {
            for (int id : ancestorHeaderIds) collapsed_.erase(id);
            break;
        }
    }
    refilter();
    for (size_t i = 0; i < shown_.size(); ++i)
        if (shown_[i].kind == Row::Item && shown_[i].display == displayClass) {
            list_.selectRow((int) i);
            list_.scrollToEnsureRowIsOnscreen((int) i);
            return;
        }
}

std::vector<std::string> HelpBrowser::indexDisplaysForTest() const {
    std::vector<std::string> out;
    for (const auto& e : outline_)
        if (e.kind == Row::Item) out.push_back(e.display);
    return out;
}

void HelpBrowser::setSearchText(const juce::String& q) {
    search_.setText(q, juce::dontSendNotification);
    refilter();
}

void HelpBrowser::resized() {
    auto r = getLocalBounds().reduced(10);
    auto left = r.removeFromLeft(230);
    search_.setBounds(left.removeFromTop(28));
    left.removeFromTop(6);
    list_.setBounds(left);
    r.removeFromLeft(10);
    heading_.setBounds(r.removeFromTop(22));
    r.removeFromTop(4);
    viewport_.setBounds(r);
    if (body_) body_->layoutTo(viewport_.getMaximumVisibleWidth());
}

void HelpBrowser::buildOutline() {
    std::set<std::string> seenDisplay;
    for (const auto& origin : classPickerGroups()) {
        if (origin.first == "Plugins") continue;
        Row oh;
        oh.kind = Row::Header;
        oh.level = 0;
        oh.id = (int) outline_.size();
        oh.label = juce::String::fromUTF8(origin.first.c_str());
        outline_.push_back(std::move(oh));

        std::vector<std::string> prev;
        for (const auto& group : origin.second) {
            const auto segs = categorySegments(group.first);
            size_t common = 0;
            while (common < segs.size() && common < prev.size()
                   && segs[common] == prev[common])
                ++common;
            for (size_t i = common; i < segs.size(); ++i) {
                Row h;
                h.kind = Row::Header;
                h.level = 1 + (int) i;
                h.id = (int) outline_.size();
                h.label = juce::String::fromUTF8(segs[i].c_str());
                outline_.push_back(std::move(h));
            }
            prev = segs;

            const auto tag = juce::String::fromUTF8(origin.first.c_str())
                           + juce::String::fromUTF8(" \xe2\x80\xba ")
                           + juce::String::fromUTF8(group.first.c_str());
            for (const auto& cls : group.second) {
                Row e;
                e.display = parseClassString(cls).display;
                if (!seenDisplay.insert(e.display).second) continue;
                e.level = 1 + (int) segs.size();
                e.label = juce::String::fromUTF8(e.display.c_str());
                e.tag = tag;
                outline_.push_back(std::move(e));
            }
        }
    }
    collapseSharedPages();
}

void HelpBrowser::collapseSharedPages() {
    std::vector<std::string> displays, docs;
    for (const auto& e : outline_)
        if (e.kind == Row::Item) {
            displays.push_back(e.display);
            docs.push_back(helpDocClass(e.display));
        }
    rowOf_ = helpIndexRows(displays, docs);
    std::vector<Row> kept;
    for (auto& e : outline_) {
        if (e.kind == Row::Item) {
            const auto it = rowOf_.find(e.display);
            if (it != rowOf_.end() && it->second != e.display) continue;
        }
        kept.push_back(std::move(e));
    }
    std::vector<bool> hasItems(kept.size(), false);
    for (size_t i = kept.size(); i-- > 0;) {
        if (kept[i].kind != Row::Header) continue;
        for (size_t j = i + 1; j < kept.size(); ++j) {
            if (kept[j].level <= kept[i].level && kept[j].kind == Row::Header) break;
            if (kept[j].kind == Row::Item) { hasItems[i] = true; break; }
        }
    }
    outline_.clear();
    for (size_t i = 0; i < kept.size(); ++i)
        if (kept[i].kind == Row::Item || hasItems[i]) outline_.push_back(std::move(kept[i]));
}

void HelpBrowser::appendVisibleOutline() {
    int collapseLevel = -1;
    for (const auto& e : outline_) {
        if (collapseLevel >= 0 && e.level > collapseLevel) continue;
        if (e.kind == Row::Header)
            collapseLevel = collapsed_.count(e.id) ? e.level : -1;
        shown_.push_back(e);
    }
}

void HelpBrowser::toggleHeader(const Row& h) {
    if (!collapsed_.erase(h.id)) collapsed_.insert(h.id);
    const int sel = list_.getSelectedRow();
    const std::string keep = (sel >= 0 && sel < (int) shown_.size())
                                 ? shown_[(size_t) sel].display
                                 : std::string();
    shown_.clear();
    appendVisibleOutline();
    list_.updateContent();
    int row = -1;
    if (!keep.empty())
        for (size_t i = 0; i < shown_.size(); ++i)
            if (shown_[i].kind == Row::Item && shown_[i].display == keep) {
                row = (int) i;
                break;
            }
    if (row >= 0)
        list_.selectRow(row);
    else
        list_.deselectAllRows();
    list_.repaint();
}

void HelpBrowser::refilter() {
    const auto q = search_.getText().toStdString();
    searching_ = !q.empty();
    shown_.clear();
    if (!searching_) {
        appendVisibleOutline();
    } else {
        const auto qLower = search_.getText().trim().toLowerCase();
        for (const auto& e : outline_) {
            if (e.kind != Row::Item) continue;
            int score = fuzzyScoreNameThenMeta(q, e.display, e.tag.toStdString());
            if (score < 0 && qLower.length() >= 3
                && pageTextOf(e.display).contains(qLower))
                score = 10;
            if (score < 0) continue;
            Row s = e;
            s.score = score;
            shown_.push_back(std::move(s));
        }
        std::stable_sort(shown_.begin(), shown_.end(), [](const Row& a, const Row& b) {
            return a.score != b.score ? a.score > b.score : a.label < b.label;
        });
    }
    list_.updateContent();
    const int first = firstItemRow();
    if (first >= 0)
        list_.selectRow(first);
    else
        list_.deselectAllRows();
    list_.repaint();
}

const juce::String& HelpBrowser::pageTextOf(const std::string& display) {
    auto it = pageText_.find(display);
    if (it != pageText_.end()) return it->second;
    const auto doc = loadOrganismDoc(display);
    juce::String flat;
    for (const auto& b : help_detail::parseHelpDoc(doc, juce::String(display)).second)
        flat += " " + b.a + " " + b.b;
    return pageText_.emplace(display, flat.toLowerCase()).first->second;
}

int HelpBrowser::firstItemRow() const {
    for (size_t i = 0; i < shown_.size(); ++i)
        if (shown_[i].kind == Row::Item) return (int) i;
    return -1;
}

void HelpBrowser::render(const std::string& displayClass) {
    heading_.setText(juce::String::fromUTF8(displayClass.c_str())
                         + juce::String(" - Help"),
                     juce::dontSendNotification);
    juce::Component::SafePointer<HelpBrowser> safe(this);
    body_ = std::make_unique<HelpPage>(
        previewFor(displayClass), nodeFor(displayClass),
        loadOrganismDoc(displayClass), displayClass,
        [safe](const std::string& c) {
            juce::MessageManager::callAsync([safe, c] {
                if (safe != nullptr) safe->showClass(c);
            });
        });
    viewport_.setViewedComponent(body_.get(), false);
    body_->layoutTo(viewport_.getMaximumVisibleWidth());
    viewport_.setViewPosition(0, 0);
}

juce::Image HelpBrowser::previewFor(const std::string& displayClass) {
    if (const auto it = previews_.find(displayClass); it != previews_.end())
        return it->second;
    juce::Image img;
    if (!previewHost_) previewHost_ = std::make_unique<EngineHost>();
    const auto name = previewHost_->addOrganism(displayClass, {0, 0});
    if (!name.empty()) {
        if (auto editor = makeOrganismEditor(*previewHost_, name)) {
            const int ew = editor->preferredContentWidth();
            const int eh = editor->preferredContentHeight(ew);
            if (ew > 0 && eh > 0) {
                editor->setSize(ew, eh);
                img = juce::Image(juce::Image::ARGB, ew, eh, true);
                juce::Graphics g(img);
                g.fillAll(Palette::panel);
                editor->paintEntireComponent(g, true);
            }
        }
        previewHost_->removeOrganism(name);
    }
    previews_[displayClass] = img;
    return img;
}

juce::Image HelpBrowser::nodeFor(const std::string& displayClass) {
    if (const auto it = nodes_.find(displayClass); it != nodes_.end())
        return it->second;
    juce::Image img;
    if (!previewHost_) previewHost_ = std::make_unique<EngineHost>();
    const auto name = previewHost_->addOrganism(displayClass, {40, 40});
    if (!name.empty()) {
        PatcherCanvas canvas(*previewHost_);
        canvas.setBounds(0, 0, 480, 360);
        canvas.refresh();
        const auto nb = canvas.nodeBounds(name).expanded(16)
                            .getIntersection(canvas.getLocalBounds());
        if (!nb.isEmpty()) img = canvas.createComponentSnapshot(nb, true, 2.0f);
        previewHost_->removeOrganism(name);
    }
    nodes_[displayClass] = img;
    return img;
}

void HelpBrowser::selectedRowsChanged(int row) {
    if (row >= 0 && row < (int) shown_.size() && shown_[(size_t) row].kind == Row::Item)
        render(shown_[(size_t) row].display);
}

void HelpBrowser::listBoxItemClicked(int row, const juce::MouseEvent&) {
    if (row < 0 || row >= (int) shown_.size()) return;
    if (!searching_ && shown_[(size_t) row].kind == Row::Header) {
        toggleHeader(shown_[(size_t) row]);
        return;
    }
    selectedRowsChanged(row);
}

void HelpBrowser::paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) {
    if (shown_.empty()) {
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(12.0f));
        g.drawText(tr("help-browser.no-matches", "no matches"), 10, 0, w - 20, h, juce::Justification::centredLeft);
        return;
    }
    if (row < 0 || row >= (int) shown_.size()) return;
    const auto& e = shown_[(size_t) row];
    const int indent = 8 + e.level * 12;

    if (e.kind == Row::Header) {
        const bool closed = collapsed_.count(e.id) > 0;
        const auto arrow = juce::String::fromUTF8(closed ? "\xe2\x96\xb8 " : "\xe2\x96\xbe ");
        g.setColour(e.level == 0 ? Palette::accent : Palette::textDim);
        g.setFont(juce::FontOptions(e.level == 0 ? 12.5f : 11.5f).withStyle("Bold"));
        g.drawText(arrow + e.label, indent, 0, w - indent - 8, h,
                   juce::Justification::centredLeft);
        return;
    }
    if (selected) {
        g.setColour(Palette::accent.withAlpha(alpha::scrim));
        g.fillRoundedRectangle(2.0f, 1.0f, (float) w - 4.0f, (float) h - 2.0f, 4.0f);
    }
    int textLeft = indent, textRight = w - 8;
    if (searching_) {
        g.setFont(juce::FontOptions(11.0f));
        g.setColour(Palette::textDim);
        const int tagW = juce::jmin(w / 2, 120);
        g.drawText(e.tag, w - tagW - 6, 0, tagW, h, juce::Justification::centredRight);
        textLeft = 8;
        textRight = w - tagW - 16;
    }
    g.setFont(juce::FontOptions(13.0f));
    g.setColour(Palette::text);
    g.drawText(e.label, textLeft, 0, textRight - textLeft, h,
               juce::Justification::centredLeft);
}

}
