// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <functional>
#include <set>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/packs/PackRegistry.h"
#include "gui/style/Colours.h"
#include "gui/style/IconGlyph.h"
#include "gui/style/LookAndFeel.h"
#include "io/PatchDocument.h"
#include "gui/common/Localisation.h"

namespace hum::caution {

inline std::string textFor(const std::string& className) {
    const auto* m = PackRegistry::instance().classManifest(className);
    return m != nullptr ? m->caution : std::string();
}

inline IconGlyph iconFor(const std::string& className) {
    const auto* m = PackRegistry::instance().classManifest(className);
    if (m != nullptr)
        for (size_t i = 0; i < kIconGlyphNames.size(); ++i)
            if (m->cautionIcon == kIconGlyphNames[i]) return (IconGlyph) i;
    return IconGlyph::Warning;
}

inline std::set<std::string>& acknowledged() {
    static std::set<std::string> s;
    return s;
}

inline bool pending(const std::string& className) {
    return !textFor(className).empty() && acknowledged().count(className) == 0;
}

inline void acknowledge(const std::string& className) { acknowledged().insert(className); }

inline std::vector<std::string> pendingIn(const PatchDocumentModel& model) {
    std::vector<std::string> out;
    for (const auto& c : model.organisms) {
        const auto& cls = c.displayClass.empty() ? c.classRaw : c.displayClass;
        if (!pending(cls)) continue;
        bool listed = false;
        for (const auto& o : out) listed = listed || o == cls;
        if (!listed) out.push_back(cls);
    }
    return out;
}

class Veil : public juce::Component {
public:
    explicit Veil(std::string className) : className_(std::move(className)) {
        setInterceptsMouseClicks(true, true);
        ok_.setButtonText(tr("caution.ok", "OK"));
        ok_.onClick = [this] {
            acknowledge(className_);
            if (onAcknowledged) onAcknowledged();
        };
        addAndMakeVisible(ok_);
    }

    std::function<void()> onAcknowledged;

    IconGlyph glyph() const { return iconFor(className_); }

    void paint(juce::Graphics& g) override {
        g.fillAll(Palette::panel.withAlpha(alpha::nearOpaque));
        const auto icon = getLocalBounds().toFloat().removeFromTop(kIconBand)
                              .withSizeKeepingCentre(kIconSize, kIconSize);
        g.setColour(Palette::text);
        drawIconGlyph(g, glyph(), icon, Palette::text, true);
        const auto r = textArea().toFloat();
        const juce::String text(textFor(className_));
        juce::AttributedString as(text);
        as.setJustification(juce::Justification::centred);
        as.setColour(Palette::text);
        as.setFont(juce::FontOptions(fittingFontSize(text, r)));
        juce::TextLayout layout;
        layout.createLayout(as, r.getWidth());
        layout.draw(g, r.withSizeKeepingCentre(r.getWidth(), std::min(r.getHeight(), layout.getHeight())));
    }

    void resized() override {
        ok_.setBounds(getLocalBounds().removeFromBottom(34).withSizeKeepingCentre(72, 24));
    }

    static float fittingFontSize(const juce::String& text, juce::Rectangle<float> area) {
        for (float size = kMaxFont; size > kMinFont; size -= 1.0f) {
            juce::AttributedString as(text);
            as.setFont(juce::FontOptions(size));
            juce::TextLayout layout;
            layout.createLayout(as, area.getWidth());
            if (layout.getHeight() <= area.getHeight()) return size;
        }
        return kMinFont;
    }

private:
    static constexpr float kMaxFont = 16.0f;
    static constexpr float kMinFont = 12.0f;
    static constexpr float kIconBand = 40.0f;
    static constexpr float kIconSize = 26.0f;

    juce::Rectangle<int> textArea() const {
        auto r = getLocalBounds().reduced(14, 4);
        r.removeFromTop((int) kIconBand);
        r.removeFromBottom(34);
        return r;
    }

    std::string className_;
    juce::TextButton ok_;
};

}
