// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include <HumBuildId.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/app/AppSettings.h"
#include "gui/app/BrandPanel.h"
#include "gui/style/Colours.h"
#include "gui/app/DemoPatches.h"
#include "gui/app/LicenseStore.h"
#include "gui/app/RecentFiles.h"
#include "gui/app/StartHereList.h"
#include "io/AutosaveStore.h"
#include "gui/common/Localisation.h"

namespace hum {

class StartWindow : public juce::Component {
public:
    struct Actions {
        std::function<void()> onNewSession;
        std::function<void(juce::File)> onOpenFile;
        std::function<void()> onOpenOther;
        std::function<void()> onDismiss;
        std::function<void(const AutosaveStore::Recovery&, bool restore)> onRecover;
        std::function<void()> onEnterLicense;
        std::function<void()> onTour;
        std::function<void()> onWizard;
        std::function<void()> onHelp;
    };

    explicit StartWindow(Actions actions, std::vector<AutosaveStore::Recovery> recover = {},
                         juce::StringArray recent = recents::get())
        : actions_(std::move(actions)), recover_(std::move(recover)), recent_(std::move(recent)) {
        if (recent_.size() > kMaxRecents) recent_.removeRange(kMaxRecents, recent_.size());

        auto styleButton = [this](juce::TextButton& b, std::function<void()> fn) {
            b.setColour(juce::TextButton::buttonColourId, ink::brand::line);
            b.setColour(juce::TextButton::textColourOffId, ink::brand::ground);
            b.onClick = std::move(fn);
            addAndMakeVisible(b);
        };
        styleButton(newBtn_, [this] { if (actions_.onNewSession) actions_.onNewSession(); });
        styleButton(openBtn_, [this] { if (actions_.onOpenOther) actions_.onOpenOther(); });
        if (actions_.onDismiss)
            styleButton(closeBtn_, [this] { actions_.onDismiss(); });
        else
            styleButton(quitBtn_, [] {
                juce::JUCEApplication::getInstance()->systemRequestedQuit();
            });

        startHere_ = std::make_unique<StartHereList>(ink::brand::line, startRows());
        addAndMakeVisible(*startHere_);

        setWantsKeyboardFocus(true);
        setSize(kW, windowHeight());
    }

    juce::String newButtonText() const { return newBtn_.getButtonText(); }
    const StartHereList& startHere() const { return *startHere_; }
    StartHereList& startHere() { return *startHere_; }

    void resized() override {
        const int bx = 30, bw = kLeftW - 2 * bx, top = leftTop();
        newBtn_.setBounds(bx, top + 270, bw, 32);
        openBtn_.setBounds(bx, top + 310, bw, 32);
        (actions_.onDismiss ? closeBtn_ : quitBtn_).setBounds(bx, top + 350, bw, 32);
        const auto f = flow();
        startHere_->setBounds(kRightX, f.startList, rightWidth(), startHere_->preferredHeight());
    }

    void paint(juce::Graphics& g) override {
        const juce::Colour bg = ink::brand::ground, line = ink::brand::line;
        paintBrandPanel(g, getLocalBounds(), bg, line);
        paintBrand(g, line);
        g.setColour(line.withAlpha(alpha::mist));
        g.fillRect(kLeftW, 16, 1, getHeight() - 32);

        const auto f = flow();
        if (!recover_.empty()) paintRecovered(g, f);
        if (!recent_.isEmpty()) paintRecents(g, line, f);
        if (startHere_->rowCount() > 0) heading(g, line, tr("start.start-here", "START HERE"), f.startHead);
        if (recent_.isEmpty() && startHere_->rowCount() == 0) {
            g.setColour(line.withAlpha(alpha::dim));
            g.setFont(juce::Font(juce::FontOptions(13.0f)));
            g.drawText(tr("start.nothing-yet-make-some-noise", "nothing yet - make some noise"), kRightX, f.recentGrid, rightWidth(),
                       kCellH, juce::Justification::centredLeft);
        }
    }

    void mouseMove(const juce::MouseEvent& e) override {
        const int h = cellAt(e.getPosition());
        const int hr = recoverRowAt(e.getPosition());
        if (h != hover_ || hr != hoverRecover_) { hover_ = h; hoverRecover_ = hr; repaint(); }
        const bool onLicense = actions_.onEnterLicense
                               && licenseLine().contains(e.getPosition());
        setMouseCursor(h >= 0 || hr >= 0 || onLicense
                           ? juce::MouseCursor::PointingHandCursor
                           : juce::MouseCursor::NormalCursor);
    }
    void mouseExit(const juce::MouseEvent&) override { hover_ = -1; hoverRecover_ = -1; repaint(); }
    void mouseUp(const juce::MouseEvent& e) override {
        if (const int ri = recoverRowAt(e.getPosition()); ri >= 0) {
            const bool discard = e.getPosition().x > recoverRowBounds(ri).getRight() - 78;
            const auto rec = recover_[(size_t) ri];
            if (discard) {
                recover_.erase(recover_.begin() + ri);
                hoverRecover_ = -1;
                if (actions_.onRecover) actions_.onRecover(rec, false);
                setSize(kW, windowHeight());
                repaint();
            } else if (actions_.onRecover) {
                actions_.onRecover(rec, true);
            }
            return;
        }
        const int i = cellAt(e.getPosition());
        if (i >= 0 && actions_.onOpenFile) actions_.onOpenFile(juce::File(recent_[i]));
        if (actions_.onEnterLicense && licenseLine().contains(e.getPosition())
            && !LicenseStore::current().valid)
            actions_.onEnterLicense();
    }
    bool keyPressed(const juce::KeyPress& k) override {
        if (k.getKeyCode() == juce::KeyPress::escapeKey && actions_.onDismiss) {
            actions_.onDismiss();
            return true;
        }
        return false;
    }

    juce::Rectangle<int> licenseLine() const { return {0, leftTop() + 250, kLeftW, 14}; }

    juce::Rectangle<int> cellBoundsForTest(int i) const { return cellBounds(i); }
    juce::Rectangle<int> leftBlockForTest() const { return {0, leftTop(), kLeftW, kLeftBlockH}; }
    int recentCountForTest() const { return recent_.size(); }

private:
    static constexpr int kW = 720, kLeftW = 220, kRightX = kLeftW + 22;
    static constexpr int kTop = 26, kRowH = 26, kCellH = 42, kCellGap = 8;
    static constexpr int kMaxRecents = 6, kMaxDemos = 4, kHeadH = 22;
    static constexpr int kLeftBlockH = 382;

    struct Flow { int recentHead, recentGrid, startHead, startList, bottom; };

    std::vector<StartHereList::Row> startRows() const {
        std::vector<StartHereList::Row> rows;
        if (actions_.onTour)
            rows.push_back({tr("start.meet-humus", "Meet Humus"), tr("start.enter-the-guided-tour", "enter the guided tour"), actions_.onTour});
        if (actions_.onWizard)
            rows.push_back({tr("start.setup-wizard", "Setup Wizard"), tr("start.choose-your-audio-and-midi", "choose your audio and MIDI devices"),
                            actions_.onWizard});
        if (actions_.onOpenFile) {
            const auto demos = demoPatches();
            for (int i = 0; i < demos.size() && i < kMaxDemos; ++i) {
                const auto f = demos[i];
                rows.push_back({demoPatchTitle(f), demoPatchBlurb(f),
                                [this, f] { actions_.onOpenFile(f); }});
            }
        }
        if (actions_.onHelp)
            rows.push_back({tr("start.help", "Help"), tr("start.explore-the-full-documentation", "explore the full documentation"), actions_.onHelp});
        return rows;
    }

    Flow flow() const {
        Flow f{};
        int y = kTop;
        if (!recover_.empty()) y += 20 + (int) recover_.size() * kRowH + 30;
        f.startHead = y;
        f.startList = y + kHeadH;
        const int listH = startHere_ != nullptr ? startHere_->preferredHeight() : 0;
        if (listH > 0) y = f.startList + listH + 18;
        f.recentHead = y;
        f.recentGrid = y + kHeadH;
        if (!recent_.isEmpty()) y = f.recentGrid + recentRows() * (kCellH + kCellGap);
        f.bottom = y + 20;
        return f;
    }

    void paintBrand(juce::Graphics& g, juce::Colour line) const {
        const int top = leftTop();
        drawHumusLogo(g, juce::Rectangle<float>(25.0f, (float) top, (float) kLeftW - 50.0f,
                                                218.0f));
        g.setColour(line.withAlpha(alpha::mid));
        g.setFont(juce::Font(juce::FontOptions(11.0f)));
        juce::String ver;
        if (auto* app = juce::JUCEApplication::getInstance())
            ver << "v" << app->getApplicationVersion();
        g.drawText(ver, 0, top + 222, kLeftW, 14, juce::Justification::centred);

        if (const juce::String build(HUM_BUILD_ID); build != ver) {
            g.setColour(line.withAlpha(alpha::dim));
            g.setFont(juce::Font(juce::FontOptions(9.5f)));
            g.drawText(build, 0, top + 236, kLeftW, 12, juce::Justification::centred);
        }

        const auto lic = LicenseStore::current();
        g.setColour(line.withAlpha(lic.valid ? 0.6f : 0.8f));
        g.setFont(juce::Font(juce::FontOptions(10.0f)));
        juce::String text;
        if (lic.valid)
            text = tr("start.registered-to", "Registered to") + " " + juce::String(lic.name.empty() ? lic.plan : lic.name);
        else
            text = tr("start.unregistered", "Unregistered") + juce::String::fromUTF8(" \xc2\xb7 ")
                   + (actions_.onEnterLicense
                          ? tr("start.enter-license", "enter license") + juce::String::fromUTF8("\xe2\x80\xa6")
                          : tr("start.fully-functional", "fully functional"));
        g.drawText(text, licenseLine(), juce::Justification::centred);
    }

    void heading(juce::Graphics& g, juce::Colour line, const juce::String& text, int y) const {
        g.setColour(line.withAlpha(alpha::dim));
        g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
        g.drawText(text, kRightX, y, rightWidth(), 16, juce::Justification::centredLeft);
    }

    void paintRecovered(juce::Graphics& g, const Flow&) const {
        const juce::Colour amber = ink::brand::recovered, line = ink::brand::line;
        g.setColour(amber);
        g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
        g.drawText(tr("start.recovered-sessions", "RECOVERED SESSIONS"), kRightX, kTop, rightWidth(), 16,
                   juce::Justification::centredLeft);
        for (int i = 0; i < (int) recover_.size(); ++i) {
            const auto& rec = recover_[(size_t) i];
            const auto r = recoverRowBounds(i);
            g.setColour(amber.withAlpha(i == hoverRecover_ ? 0.25f : 0.12f));
            g.fillRoundedRectangle(r.toFloat(), 4.0f);
            g.setColour(line);
            g.setFont(juce::Font(juce::FontOptions(14.0f)));
            const juce::String nm = rec.originalPath.isEmpty()
                ? tr("start.untitled", "Untitled") : juce::File(rec.originalPath).getFileNameWithoutExtension();
            g.drawText(nm + "  " + tr("start.unsaved-changes", "(unsaved changes)"), r.getX() + 8, r.getY(),
                       r.getWidth() - 90, r.getHeight(), juce::Justification::centredLeft, true);
            g.setColour(line.withAlpha(alpha::mid));
            g.setFont(juce::Font(juce::FontOptions(11.0f)));
            g.drawText(tr("start.discard", "discard"), r.getRight() - 70, r.getY(), 62, r.getHeight(),
                       juce::Justification::centredRight);
        }
    }

    void paintRecents(juce::Graphics& g, juce::Colour line, const Flow& f) const {
        heading(g, line, tr("start.recent-patches", "RECENT PATCHES"), f.recentHead);
        for (int i = 0; i < recent_.size(); ++i) {
            const juce::File file(recent_[i]);
            const auto r = cellBounds(i);
            if (i == hover_) {
                g.setColour(line.withAlpha(alpha::mist));
                g.fillRoundedRectangle(r.toFloat(), 4.0f);
            }
            const auto inner = r.reduced(8, 4);
            g.setColour(line);
            g.setFont(juce::Font(juce::FontOptions(14.0f)));
            g.drawText(file.getFileNameWithoutExtension(), inner.getX(), inner.getY(),
                       inner.getWidth(), 18, juce::Justification::centredLeft, true);
            g.setColour(line.withAlpha(alpha::dim));
            g.setFont(juce::Font(juce::FontOptions(10.0f)));
            g.drawText(file.getParentDirectory().getFullPathName(), inner.getX(),
                       inner.getY() + 18, inner.getWidth(), 13,
                       juce::Justification::centredLeft, true);
        }
    }

    int rightWidth() const { return getWidth() - kRightX - 22; }
    int cellWidth() const { return (rightWidth() - kCellGap) / 2; }
    int recentRows() const { return juce::jmax(0, (recent_.size() + 1) / 2); }

    int leftTop() const {
        return juce::jmax(22, (getHeight() - kLeftBlockH) / 2);
    }
    int windowHeight() const {
        return juce::jmax(juce::jmax(kLeftBlockH + 44, flow().bottom), 428);
    }
    juce::Rectangle<int> recoverRowBounds(int i) const {
        return {kRightX, kTop + 20 + i * kRowH, rightWidth(), kRowH};
    }
    int recoverRowAt(juce::Point<int> p) const {
        for (int i = 0; i < (int) recover_.size(); ++i)
            if (recoverRowBounds(i).contains(p)) return i;
        return -1;
    }
    juce::Rectangle<int> cellBounds(int i) const {
        const int col = i % 2, row = i / 2;
        return {kRightX + col * (cellWidth() + kCellGap),
                flow().recentGrid + row * (kCellH + kCellGap), cellWidth(), kCellH};
    }
    int cellAt(juce::Point<int> p) const {
        for (int i = 0; i < recent_.size(); ++i)
            if (cellBounds(i).contains(p)) return i;
        return -1;
    }

    Actions actions_;
    std::vector<AutosaveStore::Recovery> recover_;
    juce::StringArray recent_;
    std::unique_ptr<StartHereList> startHere_;
    int hover_ = -1;
    int hoverRecover_ = -1;
    juce::TextButton newBtn_{tr("start.new-session", "New Session")}, openBtn_{tr("start.open", "Open...")},
                     closeBtn_{tr("start.close", "Close")}, quitBtn_{tr("start.quit", "Quit")};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StartWindow)
};

}
