// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/browser/BrowserEntry.h"

namespace hum::browser {

class BrowserInspector : public juce::Component {
public:
    static constexpr int kWidth = 270, kPad = 14, kStarPitch = 20, kChipH = 20;

    std::function<void(int stars)> onRate;
    std::function<void(bool on)> onFavourite;
    std::function<void(const std::string& tag)> onAddTag, onRemoveTag;
    std::function<void(const std::string& collection, bool in)> onCollection;
    std::function<void()> onReveal;

    BrowserInspector();

    void show(const Entry* entry, const std::vector<std::string>& collections,
              const std::vector<std::string>& holding, const juce::String& where);
    bool showing() const { return has_; }
    const Entry& entry() const { return entry_; }

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseUp(const juce::MouseEvent& e) override;
    void lookAndFeelChanged() override { restyle(); }
    void restyle();

    juce::TextEditor& tagEditor() { return tagEdit_; }
    int collectionToggleCount() const { return (int) toggles_.size(); }
    juce::ToggleButton* collectionToggle(int i) { return i >= 0 && i < (int) toggles_.size() ? toggles_[(size_t) i].get() : nullptr; }

private:
    struct Chip {
        std::string tag;
        juce::Rectangle<float> bounds, cross;
    };

    juce::Rectangle<float> starsArea() const;
    juce::Rectangle<float> heartArea() const;
    std::vector<Chip> chips() const;
    int tagsTop() const;
    int factsTop() const;
    std::vector<std::pair<juce::String, juce::String>> facts() const;

    Entry entry_;
    bool has_ = false;
    juce::String where_;
    std::vector<std::string> collections_;
    juce::TextEditor tagEdit_;
    std::vector<std::unique_ptr<juce::ToggleButton>> toggles_;
    juce::TextButton reveal_;
};

}
