// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/browser/BrowserInspector.h"

#include <algorithm>

#include "gui/browser/BrowserPaint.h"
#include "gui/common/Localisation.h"

namespace hum::browser {

namespace {

constexpr int kTitleH = 40, kStarsH = 26, kHeadH = 22, kFactH = 18, kRevealH = 26, kToggleH = 22;
constexpr float kBigStar = 16.0f;

void head(juce::Graphics& g, const juce::String& text, int x, int y, int w) {
    g.setColour(Palette::textDim.withAlpha(alpha::strong));
    g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    g.drawText(text, x, y, w, kHeadH, juce::Justification::bottomLeft, false);
}

}

BrowserInspector::BrowserInspector() {
    tagEdit_.setTextToShowWhenEmpty(tr("browser.add-a-tag", "+ add a tag"), Palette::textDim.withAlpha(alpha::strong));
    tagEdit_.setFont(juce::FontOptions(12.0f));
    tagEdit_.onReturnKey = [this] {
        const auto text = tagEdit_.getText().trim().toStdString();
        tagEdit_.clear();
        if (!text.empty() && onAddTag) onAddTag(text);
    };
    addAndMakeVisible(tagEdit_);
    reveal_.setButtonText(paint::revealLabel());
    reveal_.onClick = [this] { if (onReveal) onReveal(); };
    addAndMakeVisible(reveal_);
    setSize(kWidth, 400);
    restyle();
}

void BrowserInspector::restyle() {
    tagEdit_.setColour(juce::TextEditor::backgroundColourId, Palette::background);
    tagEdit_.setColour(juce::TextEditor::outlineColourId, Palette::border);
    tagEdit_.setColour(juce::TextEditor::textColourId, Palette::text);
    tagEdit_.applyColourToAllText(Palette::text);
    reveal_.setColour(juce::TextButton::buttonColourId, Palette::panelLight);
    reveal_.setColour(juce::TextButton::textColourOffId, Palette::text);
    for (auto& t : toggles_) {
        t->setColour(juce::ToggleButton::textColourId, Palette::text);
        t->setColour(juce::ToggleButton::tickColourId, Palette::accent);
    }
    repaint();
}

void BrowserInspector::show(const Entry* entry, const std::vector<std::string>& collections,
                            const std::vector<std::string>& holding, const juce::String& where) {
    has_ = entry != nullptr;
    if (has_) entry_ = *entry;
    where_ = where;
    const bool sameList = collections == collections_;
    collections_ = collections;
    if (!sameList) {
        toggles_.clear();
        for (const auto& name : collections_) {
            auto t = std::make_unique<juce::ToggleButton>(juce::String::fromUTF8(name.c_str()));
            t->onClick = [this, name, raw = t.get()] { if (onCollection) onCollection(name, raw->getToggleState()); };
            addAndMakeVisible(*t);
            toggles_.push_back(std::move(t));
        }
    }
    restyle();
    for (size_t i = 0; i < toggles_.size(); ++i)
        toggles_[i]->setToggleState(std::find(holding.begin(), holding.end(), collections_[i]) != holding.end(),
                                    juce::dontSendNotification);
    for (auto* c : getChildren()) c->setVisible(has_);
    resized();
    repaint();
}

juce::Rectangle<float> BrowserInspector::starsArea() const {
    return {(float) kPad, (float) (kPad + kTitleH), (float) (kStarPitch * kMaxRating), (float) kStarsH};
}

juce::Rectangle<float> BrowserInspector::heartArea() const {
    return {(float) (getWidth() - kPad - kStarsH), (float) (kPad + kTitleH), (float) kStarsH, (float) kStarsH};
}

int BrowserInspector::tagsTop() const { return kPad + kTitleH + kStarsH + 6; }

std::vector<BrowserInspector::Chip> BrowserInspector::chips() const {
    std::vector<Chip> out;
    float x = (float) kPad, y = (float) (tagsTop() + kHeadH + 4);
    const float right = (float) (getWidth() - kPad);
    for (const auto& t : entry_.tags) {
        const auto text = "#" + juce::String::fromUTF8(t.c_str());
        const float w = juce::GlyphArrangement::getStringWidth(juce::FontOptions(11.5f), text) + 30.0f;
        if (x + w > right && x > (float) kPad) {
            x = (float) kPad;
            y += (float) kChipH + 4.0f;
        }
        Chip c;
        c.tag = t;
        c.bounds = {x, y, w, (float) kChipH};
        c.cross = c.bounds.removeFromRight(16.0f);
        c.bounds = {x, y, w, (float) kChipH};
        out.push_back(c);
        x += w + 4.0f;
    }
    return out;
}

int BrowserInspector::factsTop() const {
    const auto cs = chips();
    const int chipsBottom = cs.empty() ? tagsTop() + kHeadH + 4 : (int) cs.back().bounds.getBottom() + 4;
    const int editBottom = chipsBottom + 4 + 24;
    const int collectionsBottom = editBottom + (collections_.empty() ? 0 : kHeadH + 4 + kToggleH * (int) collections_.size());
    return collectionsBottom + 6;
}

std::vector<std::pair<juce::String, juce::String>> BrowserInspector::facts() const {
    std::vector<std::pair<juce::String, juce::String>> out;
    auto add = [&](const juce::String& k, const juce::String& v) { if (v.isNotEmpty()) out.push_back({k, v}); };
    add(tr("browser.fact-where", "Where"), where_);
    add(tr("browser.fact-length", "Length"), paint::lengthText(entry_.facts.seconds));
    add(tr("browser.fact-rate", "Rate"), paint::rateText(entry_.facts.sampleRate));
    add(tr("browser.fact-channels", "Channels"), entry_.facts.channels > 0 ? juce::String(entry_.facts.channels) : juce::String());
    add(tr("browser.fact-bpm", "BPM"), entry_.facts.bpm > 0.0 ? juce::String(entry_.facts.bpm, 1) : juce::String());
    add(tr("browser.fact-key", "Key"), juce::String::fromUTF8(entry_.facts.key.c_str()));
    add(tr("browser.fact-size", "Size"), paint::sizeText(entry_.size));
    add(tr("browser.fact-created", "Created"), paint::dateText(entry_.created));
    add(tr("browser.fact-modified", "Modified"), paint::dateText(entry_.modified));
    add(tr("browser.fact-added", "Added"), paint::dateText(entry_.added));
    add(tr("browser.fact-used", "Last used"), paint::dateText(entry_.lastUsed));
    return out;
}

void BrowserInspector::paint(juce::Graphics& g) {
    g.fillAll(Palette::panel);
    g.setColour(Palette::border.withAlpha(alpha::strong));
    g.drawVerticalLine(0, 0.0f, (float) getHeight());
    const int w = getWidth() - 2 * kPad;
    if (!has_) {
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(12.0f));
        g.drawFittedText(tr("browser.inspector-empty", "Select a file to see and edit its details"),
                         kPad, kPad, w, 60, juce::Justification::topLeft, 3);
        return;
    }
    paint::drawBadge(g, entry_.kind, (float) kPad, (float) kPad + 8.0f);
    g.setColour(Palette::text);
    g.setFont(juce::FontOptions(13.5f, juce::Font::bold));
    g.drawFittedText(juce::String::fromUTF8(fileName(entry_.path).c_str()), kPad, kPad + 18, w, kTitleH - 18,
                     juce::Justification::topLeft, 1);
    const auto stars = starsArea();
    for (int i = 0; i < kMaxRating; ++i) {
        juce::Path star;
        star.addStar({stars.getX() + kStarPitch * (float) i + kBigStar * 0.5f, stars.getCentreY()}, 5, kBigStar * 0.22f, kBigStar * 0.5f);
        const bool lit = i < entry_.rating;
        g.setColour(lit ? Palette::text : Palette::textDim.withAlpha(alpha::mid));
        if (lit) g.fillPath(star);
        else g.strokePath(star, juce::PathStrokeType(1.0f));
    }
    paint::drawHeart(g, heartArea(), entry_.favourite, true);
    head(g, tr("browser.tags", "TAGS"), kPad, tagsTop(), w);
    for (const auto& c : chips()) {
        g.setColour(Palette::panelLight);
        g.fillRoundedRectangle(c.bounds, 4.0f);
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(11.5f));
        g.drawText("#" + juce::String::fromUTF8(c.tag.c_str()), c.bounds.withTrimmedLeft(7.0f).withTrimmedRight(16.0f),
                   juce::Justification::centredLeft, true);
        g.setColour(Palette::textDim);
        g.drawText("x", c.cross, juce::Justification::centred, false);
    }
    if (!collections_.empty())
        head(g, tr("browser.collections-head", "COLLECTIONS"), kPad, tagEdit_.getBottom() + 4, w);
    int y = factsTop();
    head(g, tr("browser.details", "DETAILS"), kPad, y, w);
    y += kHeadH + 4;
    g.setFont(juce::FontOptions(11.5f));
    for (const auto& [k, v] : facts()) {
        g.setColour(Palette::textDim);
        g.drawText(k, kPad, y, 72, kFactH, juce::Justification::centredLeft, false);
        g.setColour(Palette::text);
        g.drawText(v, kPad + 76, y, w - 76, kFactH, juce::Justification::centredLeft, true);
        y += kFactH;
    }
}

void BrowserInspector::resized() {
    const auto cs = chips();
    const int chipsBottom = cs.empty() ? tagsTop() + kHeadH + 4 : (int) cs.back().bounds.getBottom() + 4;
    tagEdit_.setBounds(kPad, chipsBottom + 4, getWidth() - 2 * kPad, 24);
    int y = tagEdit_.getBottom() + 4 + kHeadH + 4;
    for (auto& t : toggles_) {
        t->setBounds(kPad - 4, y, getWidth() - 2 * kPad, kToggleH);
        y += kToggleH;
    }
    reveal_.setBounds(kPad, getHeight() - kPad - kRevealH, getWidth() - 2 * kPad, kRevealH);
}

void BrowserInspector::mouseUp(const juce::MouseEvent& e) {
    if (!has_) return;
    if (starsArea().contains(e.position) && onRate) {
        const int stars = std::clamp((int) ((e.position.x - starsArea().getX()) / (float) kStarPitch) + 1, 1, kMaxRating);
        onRate(stars == entry_.rating ? 0 : stars);
        return;
    }
    if (heartArea().contains(e.position) && onFavourite) {
        onFavourite(!entry_.favourite);
        return;
    }
    for (const auto& c : chips())
        if (c.cross.contains(e.position) && onRemoveTag) {
            onRemoveTag(c.tag);
            return;
        }
}

}
