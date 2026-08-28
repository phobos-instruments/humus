#pragma once
#include <functional>
#include <vector>

#include <BinaryData.h>
#include <HumBuildId.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/AppSettings.h"
#include "gui/RecentFiles.h"
#include "gui/LicenseStore.h"
#include "io/AutosaveStore.h"

namespace hum {

inline const juce::Image& humusLogo() {
    static const juce::Image img = juce::ImageCache::getFromMemory(
        BinaryData::logo_png, BinaryData::logo_pngSize);
    return img;
}

inline constexpr float kBrandRadius = 12.0f;

inline void paintBrandPanel(juce::Graphics& g, juce::Rectangle<int> bounds,
                            juce::Colour fill, juce::Colour edge) {
    juce::Graphics::ScopedSaveState keep(g);
    const auto r = bounds.toFloat().reduced(0.5f);
    g.setColour(fill);
    g.fillRoundedRectangle(r, kBrandRadius);
    g.setColour(edge.withAlpha(0.18f));
    g.drawRoundedRectangle(r, kBrandRadius, 1.0f);
}

inline void drawHumusLogo(juce::Graphics& g, juce::Rectangle<float> bounds) {
    juce::Graphics::ScopedSaveState keep(g);
    g.setColour(juce::Colours::white);
    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    g.drawImage(humusLogo(), bounds, juce::RectanglePlacement::centred);
}

class StartWindow : public juce::Component {
public:
    struct Actions {
        std::function<void()> onNewSession;
        std::function<void(juce::File)> onOpenFile;
        std::function<void()> onOpenOther;
        std::function<void()> onDismiss;
        std::function<void(const AutosaveStore::Recovery&, bool restore)> onRecover;
        std::function<void()> onEnterLicense;
    };

    explicit StartWindow(Actions actions, std::vector<AutosaveStore::Recovery> recover = {})
        : actions_(std::move(actions)), recover_(std::move(recover)) {
        recent_ = recents::get();
        if (recent_.size() > kMaxRecents) recent_.removeRange(kMaxRecents, recent_.size());

        auto styleButton = [this](juce::TextButton& b, std::function<void()> fn) {
            b.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff33402a));
            b.setColour(juce::TextButton::textColourOffId, juce::Colour(0xfff4ecdc));
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
        setWantsKeyboardFocus(true);
        setSize(kW, windowHeight());
    }

    void resized() override {
        const int bx = 30, bw = kLeftW - 2 * bx;
        newBtn_.setBounds(bx, 292, bw, 32);
        openBtn_.setBounds(bx, 332, bw, 32);
        (actions_.onDismiss ? closeBtn_ : quitBtn_).setBounds(bx, 372, bw, 32);
    }

    void paint(juce::Graphics& g) override {
        const juce::Colour bg(0xfff4ecdc), line(0xff33402a);
        paintBrandPanel(g, getLocalBounds(), bg, line);

        drawHumusLogo(g, juce::Rectangle<float>(25.0f, 22.0f, (float) kLeftW - 50.0f, 218.0f));
        g.setColour(line.withAlpha(0.6f));
        g.setFont(juce::Font(juce::FontOptions(11.0f)));
        juce::String ver;
        if (auto* app = juce::JUCEApplication::getInstance())
            ver << "v" << app->getApplicationVersion();
        g.drawText(ver, 0, 244, kLeftW, 14, juce::Justification::centred);

        if (const juce::String build(HUM_BUILD_ID); build != ver) {
            g.setColour(line.withAlpha(0.4f));
            g.setFont(juce::Font(juce::FontOptions(9.5f)));
            g.drawText(build, 0, 258, kLeftW, 12, juce::Justification::centred);
        }

        {
            const auto lic = LicenseStore::current();
            g.setColour(line.withAlpha(lic.valid ? 0.6f : 0.8f));
            g.setFont(juce::Font(juce::FontOptions(10.0f)));
            juce::String text;
            if (lic.valid)
                text = "Registered to "
                       + juce::String(lic.name.empty() ? lic.plan : lic.name);
            else
                text = actions_.onEnterLicense
                           ? juce::String::fromUTF8("Unregistered \xc2\xb7 enter license\xe2\x80\xa6")
                           : juce::String::fromUTF8("Unregistered \xc2\xb7 fully functional");
            g.drawText(text, licenseLine(), juce::Justification::centred);
        }

        g.setColour(line.withAlpha(0.15f));
        g.fillRect(kLeftW, 16, 1, getHeight() - 32);

        if (!recover_.empty()) {
            const juce::Colour amber(0xffb07818);
            g.setColour(amber);
            g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
            g.drawText("RECOVERED SESSIONS", kRightX, kTop, rightWidth(), 16,
                       juce::Justification::centredLeft);
            for (int i = 0; i < (int) recover_.size(); ++i) {
                const auto& rec = recover_[(size_t) i];
                const auto r = recoverRowBounds(i);
                g.setColour(amber.withAlpha(i == hoverRecover_ ? 0.25f : 0.12f));
                g.fillRoundedRectangle(r.toFloat(), 4.0f);
                g.setColour(line);
                g.setFont(juce::Font(juce::FontOptions(14.0f)));
                const juce::String nm = rec.originalPath.isEmpty()
                    ? "Untitled" : juce::File(rec.originalPath).getFileNameWithoutExtension();
                g.drawText(nm + "  (unsaved changes)", r.getX() + 8, r.getY(),
                           r.getWidth() - 90, r.getHeight(), juce::Justification::centredLeft, true);
                g.setColour(line.withAlpha(0.55f));
                g.setFont(juce::Font(juce::FontOptions(11.0f)));
                g.drawText("discard", r.getRight() - 70, r.getY(), 62, r.getHeight(),
                           juce::Justification::centredRight);
            }
        }

        g.setColour(line.withAlpha(0.5f));
        g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
        g.drawText("RECENT PATCHES", kRightX, gridTop() - 22, rightWidth(), 16,
                   juce::Justification::centredLeft);
        if (recent_.isEmpty()) {
            g.setColour(line.withAlpha(0.4f));
            g.setFont(juce::Font(juce::FontOptions(13.0f)));
            g.drawText(juce::String("nothing yet - make some noise"),
                   kRightX, gridTop(), rightWidth(),
                       kCellH, juce::Justification::centredLeft);
            return;
        }
        for (int i = 0; i < recent_.size(); ++i) {
            const juce::File f(recent_[i]);
            const auto r = cellBounds(i);
            if (i == hover_) {
                g.setColour(line.withAlpha(0.12f));
                g.fillRoundedRectangle(r.toFloat(), 4.0f);
            }
            const auto inner = r.reduced(8, 4);
            g.setColour(line);
            g.setFont(juce::Font(juce::FontOptions(14.0f)));
            g.drawText(f.getFileNameWithoutExtension(), inner.getX(), inner.getY(),
                       inner.getWidth(), 18, juce::Justification::centredLeft, true);
            g.setColour(line.withAlpha(0.45f));
            g.setFont(juce::Font(juce::FontOptions(10.0f)));
            g.drawText(f.getParentDirectory().getFullPathName(), inner.getX(),
                       inner.getY() + 18, inner.getWidth(), 13,
                       juce::Justification::centredLeft, true);
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

    static juce::Rectangle<int> licenseLine() { return {0, 272, kLeftW, 14}; }

private:
    static constexpr int kW = 720, kLeftW = 220, kRightX = kLeftW + 22;
    static constexpr int kTop = 26, kRowH = 26, kCellH = 42, kCellGap = 8;
    static constexpr int kMaxRecents = 10;

    int rightWidth() const { return getWidth() - kRightX - 22; }
    int cellWidth() const { return (rightWidth() - kCellGap) / 2; }

    int gridTop() const {
        return recover_.empty() ? kTop + 22
                                : kTop + 20 + (int) recover_.size() * kRowH + 30;
    }
    int windowHeight() const {
        const int rows = juce::jmax(1, (recent_.size() + 1) / 2);
        const int rightH = gridTop() + rows * (kCellH + kCellGap) + 16;
        const int leftH = 372 + 32 + 24;
        return juce::jmax(juce::jmax(leftH, rightH), 428);
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
                gridTop() + row * (kCellH + kCellGap), cellWidth(), kCellH};
    }
    int cellAt(juce::Point<int> p) const {
        for (int i = 0; i < recent_.size(); ++i)
            if (cellBounds(i).contains(p)) return i;
        return -1;
    }

    Actions actions_;
    std::vector<AutosaveStore::Recovery> recover_;
    juce::StringArray recent_;
    int hover_ = -1;
    int hoverRecover_ = -1;
    juce::TextButton newBtn_{"New Session"}, openBtn_{"Open..."}, closeBtn_{"Close"},
                     quitBtn_{"Quit"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StartWindow)
};

}
