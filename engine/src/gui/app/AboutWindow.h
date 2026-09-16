// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <functional>
#include <utility>
#include <vector>

#include <HumBuildId.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/app/AboutCredits.h"
#include "gui/app/BrandPanel.h"
#include "gui/style/Colours.h"
#include "gui/app/LicenseStore.h"
#include "gui/common/Localisation.h"

namespace hum {

class AboutBody : public juce::Component {
public:
    static constexpr int kPad = 24, kGap = 12;

    explicit AboutBody(juce::Colour ink) : ink_(ink) {}

    int contentHeight() const { return height_; }
    int itemCount() const { return (int) items_.size(); }

    void layoutFor(int width) {
        items_.clear();
        const int textW = juce::jmax(80, width - 2 * kPad);
        int y = kGap;
        auto add = [&](juce::AttributedString a) {
            juce::TextLayout tl;
            tl.createLayout(a, (float) textW);
            const int h = (int) std::ceil(tl.getHeight());
            items_.push_back({std::move(a), y, h});
            y += h + kGap;
        };
        auto para = [&](const about::Line& line, float size, float alpha) {
            juce::AttributedString a;
            a.append(tr(line.key, line.text), juce::Font(juce::FontOptions(size)),
                     ink_.withAlpha(alpha));
            add(std::move(a));
        };
        para(about::kLicence, 12.5f, 0.85f);
        para(about::kTrademark, 12.5f, 0.6f);
        {
            juce::AttributedString a;
            a.append(tr("about.credits", "CREDITS"), juce::Font(juce::FontOptions(11.0f, juce::Font::bold)),
                     ink_.withAlpha(alpha::dim));
            add(std::move(a));
        }
        for (const auto& c : about::credits()) {
            juce::AttributedString a;
            a.append(c.name, juce::Font(juce::FontOptions(12.5f, juce::Font::bold)), ink_);
            a.append(juce::String("   ") + c.terms, juce::Font(juce::FontOptions(11.0f)),
                     ink_.withAlpha(alpha::dim));
            a.append("\n" + tr(c.key, c.what), juce::Font(juce::FontOptions(11.5f)),
                     ink_.withAlpha(alpha::strong));
            add(std::move(a));
        }
        para(about::kOwners, 11.0f, 0.5f);
        height_ = y;
        setSize(width, height_);
    }

    void paint(juce::Graphics& g) override {
        for (const auto& it : items_)
            it.attr.draw(g, juce::Rectangle<float>((float) kPad, (float) it.y,
                                                   (float) (getWidth() - 2 * kPad),
                                                   (float) it.h));
    }

private:
    struct Item {
        juce::AttributedString attr;
        int y = 0, h = 0;
    };

    juce::Colour ink_;
    std::vector<Item> items_;
    int height_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AboutBody)
};

class AboutWindow : public juce::Component {
public:
    struct Actions {
        std::function<void()> onDismiss;
        std::function<void()> onEnterLicense;
    };

    static constexpr int kW = 560, kH = 604, kHeadH = 276;

    explicit AboutWindow(Actions actions) : actions_(std::move(actions)) {
        auto styleButton = [this](juce::TextButton& b, std::function<void()> fn) {
            b.setColour(juce::TextButton::buttonColourId, ink::brand::line);
            b.setColour(juce::TextButton::textColourOffId, ink::brand::ground);
            b.onClick = std::move(fn);
            addAndMakeVisible(b);
        };
        styleButton(siteBtn_, [] { juce::URL(about::kSite).launchInDefaultBrowser(); });
        styleButton(sourceBtn_, [] { juce::URL(about::kSource).launchInDefaultBrowser(); });
        styleButton(closeBtn_, [this] { if (actions_.onDismiss) actions_.onDismiss(); });
        if (actions_.onEnterLicense)
            styleButton(licenseBtn_, [this] { actions_.onEnterLicense(); });

        body_.layoutFor(kW - 2);
        view_.setViewedComponent(&body_, false);
        view_.setScrollBarsShown(true, false);
        addAndMakeVisible(view_);
        setWantsKeyboardFocus(true);
        setSize(kW, kH);
    }

    juce::String siteButtonText() const { return siteBtn_.getButtonText(); }
    const AboutBody& body() const { return body_; }
    AboutBody& body() { return body_; }

    void resized() override {
        const int pad = 22, btnH = 30, btnY = getHeight() - btnH - 18;
        view_.setBounds(pad / 2, kHeadH, getWidth() - pad, btnY - kHeadH - 12);
        body_.layoutFor(view_.getMaximumVisibleWidth());

        std::vector<juce::TextButton*> btns;
        if (actions_.onEnterLicense) btns.push_back(&licenseBtn_);
        btns.push_back(&siteBtn_);
        btns.push_back(&sourceBtn_);
        btns.push_back(&closeBtn_);
        const int gap = 8;
        const int bw = (getWidth() - 2 * pad - gap * ((int) btns.size() - 1))
                       / (int) btns.size();
        for (int i = 0; i < (int) btns.size(); ++i)
            btns[(size_t) i]->setBounds(pad + i * (bw + gap), btnY, bw, btnH);
    }

    void paint(juce::Graphics& g) override {
        const juce::Colour bg = ink::brand::ground, line = ink::brand::line;
        paintBrandPanel(g, getLocalBounds(), bg, line);
        drawHumusMark(g, juce::Rectangle<float>((float) getWidth() / 2.0f - 66.0f, 12.0f,
                                                132.0f, 132.0f));

        juce::String title("Humus");
        if (auto* app = juce::JUCEApplication::getInstance())
            title << " " << app->getApplicationVersion();
        g.setColour(line);
        g.setFont(juce::Font(juce::FontOptions(17.0f, juce::Font::bold)));
        g.drawText(title, 0, 150, getWidth(), 22, juce::Justification::centred);

        g.setColour(line.withAlpha(alpha::dim));
        g.setFont(juce::Font(juce::FontOptions(10.0f)));
        g.drawText(HUM_BUILD_ID, 0, 172, getWidth(), 14, juce::Justification::centred);

        g.setColour(line.withAlpha(alpha::heavy));
        g.setFont(juce::Font(juce::FontOptions(12.5f)));
        g.drawFittedText(tr(about::kTagline.key, about::kTagline.text), 60, 192, getWidth() - 120, 34,
                         juce::Justification::centredTop, 2);

        g.setColour(line.withAlpha(alpha::mid));
        g.setFont(juce::Font(juce::FontOptions(11.5f)));
        g.drawText(tr(about::kCopyright.key, about::kCopyright.text), 0, 226, getWidth(), 14, juce::Justification::centred);
        g.drawText(tr(about::kAuthor.key, about::kAuthor.text), 0, 242, getWidth(), 14, juce::Justification::centred);

        if (const auto lic = LicenseStore::current(); lic.valid) {
            g.setColour(line.withAlpha(alpha::mid));
            g.setFont(juce::Font(juce::FontOptions(11.0f)));
            g.drawText(tr("about.registered-to", "Registered to") + " " + juce::String(lic.name.empty() ? lic.plan : lic.name),
                       0, 258, getWidth(), 14, juce::Justification::centred);
        }
    }

    bool keyPressed(const juce::KeyPress& k) override {
        if (k.getKeyCode() == juce::KeyPress::escapeKey && actions_.onDismiss) {
            actions_.onDismiss();
            return true;
        }
        return false;
    }

private:
    Actions actions_;
    AboutBody body_{ink::brand::line};
    juce::Viewport view_;
    juce::TextButton siteBtn_{tr("about.website", "Website")}, sourceBtn_{tr("about.source", "Source")},
                     closeBtn_{tr("about.close", "Close")}, licenseBtn_{tr("about.enter-license", "Enter License...")};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AboutWindow)
};

}
