#pragma once
#include <cmath>
#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/AppPaths.h"
#include "core/GuideFlow.h"
#include "gui/AppSettings.h"
#include "gui/HelpBody.h"
#include "gui/HelpMarkdown.h"
#include "gui/LookAndFeel.h"
#include "gui/DemoPatches.h"
#include "gui/StartWindow.h"
#include "gui/Localisation.h"

namespace hum {

class GuideBody : public juce::Component {
public:
    explicit GuideBody(const juce::String& raw) {
        const auto blocks = help_detail::parseMarkdown(raw);
        const juce::Font fBody(juce::FontOptions(14.5f));
        const juce::Font fName(juce::FontOptions(14.5f).withStyle("Bold"));
        const juce::Font fH[3] = {juce::Font(juce::FontOptions(19.0f).withStyle("Bold")),
                                  juce::Font(juce::FontOptions(15.5f).withStyle("Bold")),
                                  juce::Font(juce::FontOptions(14.5f).withStyle("Bold"))};
        using K = help_detail::Block;
        for (size_t i = 0; i < blocks.size(); ++i) {
            const auto& blk = blocks[i];
            Chunk c;
            if (blk.kind == K::Image) {
                c.img = juce::ImageFileFormat::loadFrom(resolveAssetRef(blk.a));
                if (c.img.isNull()) continue;
            } else if (blk.kind == K::Header) {
                const auto& f = fH[juce::jlimit(1, 3, blk.level) - 1];
                help_detail::appendRich(c.attr, blk.a, blk.rich, f, f, Palette::accent);
            } else if (blk.kind == K::Def) {
                c.attr.append(blk.a, fName, Palette::text);
                help_detail::appendRich(c.attr, "  " + blk.b, blk.rich, fBody, fName,
                                        Palette::text);
                c.tight = i > 0 && blocks[i - 1].kind == K::Def;
            } else {
                const auto dim = blk.level < 0;
                help_detail::appendRich(c.attr, blk.bullet ? juce::String::fromUTF8(
                                                    "\xe2\x80\xa2  ") + blk.a : blk.a,
                                        blk.rich, fBody, fName,
                                        dim ? Palette::textDim : Palette::text);
                c.tight = blk.bullet && i > 0 && blocks[i - 1].bullet;
            }
            chunks_.push_back(std::move(c));
        }
    }

    void layoutTo(int width, int minHeight) {
        colW_ = (float) juce::jmin(width - 24, 560);
        float y = 0.0f;
        for (auto& c : chunks_) {
            if (y > 0.0f) y += c.tight ? 3.0f : 12.0f;
            c.y = y;
            if (c.img.isValid()) {
                const float w = juce::jmin(colW_, (float) c.img.getWidth());
                c.imgH = w * (float) c.img.getHeight() / (float) c.img.getWidth();
                y += c.imgH;
                continue;
            }
            c.layout = juce::TextLayout();
            c.layout.createLayout(c.attr, colW_);
            y += c.layout.getHeight();
        }
        textH_ = (float) std::ceil(y);
        setSize(width, juce::jmax((int) textH_ + 16, minHeight));
    }
    float textHeight() const { return textH_; }

    void paint(juce::Graphics& g) override {
        const float x = ((float) getWidth() - colW_) * 0.5f;
        const float top = ((float) getHeight() - textH_) * 0.5f;
        for (auto& c : chunks_) {
            if (c.img.isValid()) {
                const float w = juce::jmin(colW_, (float) c.img.getWidth());
                g.drawImage(c.img, juce::Rectangle<float>(x + (colW_ - w) * 0.5f,
                                                          top + c.y, w, c.imgH),
                            juce::RectanglePlacement::centred);
                continue;
            }
            c.layout.draw(g, juce::Rectangle<float>(x, top + c.y, colW_,
                                                    c.layout.getHeight()));
        }
    }

private:
    struct Chunk {
        juce::AttributedString attr;
        juce::TextLayout layout;
        juce::Image img;
        float y = 0.0f, imgH = 0.0f;
        bool tight = false;
    };
    std::vector<Chunk> chunks_;
    float colW_ = 560.0f, textH_ = 0.0f;
};

class GuideView : public juce::Component {
public:
    struct Actions {
        std::function<void(juce::File)> onOpenDemo;
        std::function<void()> onWizard;
        std::function<void()> onFinished;
    };

    GuideView(Actions actions, bool firstBoot)
        : actions_(std::move(actions)), firstBoot_(firstBoot) {
        for (auto* b : {&backBtn_, &nextBtn_, &skipBtn_}) {
            b->setColour(juce::TextButton::buttonColourId, kBrandInk);
            b->setColour(juce::TextButton::textColourOffId, kBrandBg);
            addAndMakeVisible(*b);
        }
        skipBtn_.setButtonText(firstBoot_ ? tr("guide.skip-tour", "Skip tour") : tr("guide.close", "Close"));
        backBtn_.onClick = [this] { setPage(guide::back(page_)); };
        nextBtn_.onClick = [this] {
            if (guide::isLast(page_)) finish();
            else setPage(guide::next(page_));
        };
        skipBtn_.onClick = [this] { finish(); };

        demoBtn_.onClick = [this] { pickDemo(); };
        wizardBtn_.onClick = [this] {
            if (actions_.onWizard) actions_.onWizard();
            finish();
        };
        addChildComponent(demoBtn_);
        addChildComponent(wizardBtn_);
        addChildComponent(legend_);
        addChildComponent(podSketch_);

        setWantsKeyboardFocus(true);
        setSize(kW, kHeaderH + kContentH + kFooterH);
        viewport_.setScrollBarsShown(true, false);
        addAndMakeVisible(viewport_);
        setPage(guide::kWelcome);
    }

    bool bodyOverflows() const {
        return body_ != nullptr
            && body_->textHeight() > (float) viewport_.getHeight() - 8.0f;
    }
    int bodyOverflowBy() const {
        return body_ == nullptr ? 0
             : (int) std::ceil(body_->textHeight()) - (viewport_.getHeight() - 8);
    }

    void setPage(guide::Page p) {
        page_ = p;
        body_ = std::make_unique<GuideBody>(juce::String::fromUTF8(guide::pageBody(p).c_str()));
        viewport_.setViewedComponent(body_.get(), false);
        legend_.setVisible(p == guide::kPatch);
        podSketch_.setVisible(p == guide::kFold);
        const bool last = guide::isLast(p);
        demoBtn_.setVisible(last && hasDemos_);
        wizardBtn_.setVisible(last);
        backBtn_.setEnabled(!guide::isFirst(p));
        nextBtn_.setButtonText(last ? tr("guide.start-growing", "Start Growing") : tr("guide.next", "Next"));
        skipBtn_.setVisible(!last);
        resized();
        repaint();
    }

    void resized() override {
        auto content = juce::Rectangle<int>(0, kHeaderH, getWidth(), kContentH).reduced(24, 18);
        if (guide::isFirst(page_)) {
            hero_ = content.removeFromTop(kHeroH).toFloat();
            content.removeFromTop(10);
        } else {
            hero_ = {};
        }
        if (guide::isLast(page_)) {
            auto rows = content.removeFromBottom(84);
            auto place = [&rows](juce::TextButton& b) {
                b.setBounds(rows.removeFromTop(36).withSizeKeepingCentre(300, 32));
                rows.removeFromTop(8);
            };
            if (firstBoot_) { place(wizardBtn_); if (hasDemos_) place(demoBtn_); }
            else { if (hasDemos_) place(demoBtn_); place(wizardBtn_); }
            content.removeFromBottom(8);
        }
        if (legend_.isVisible()) {
            legend_.setBounds(content.removeFromBottom(60));
            content.removeFromBottom(6);
        }
        if (podSketch_.isVisible()) {
            podSketch_.setBounds(content.removeFromBottom(190));
            content.removeFromBottom(6);
        }
        viewport_.setBounds(content);
        if (body_) body_->layoutTo(viewport_.getMaximumVisibleWidth(), viewport_.getHeight());

        auto footer = getLocalBounds().removeFromBottom(kFooterH).reduced(24, 11);
        backBtn_.setBounds(footer.removeFromLeft(90));
        nextBtn_.setBounds(footer.removeFromRight(130));
        footer.removeFromRight(10);
        skipBtn_.setBounds(footer.removeFromRight(100));
    }

    void paint(juce::Graphics& g) override {
        paintBrandPanel(g, getLocalBounds(), kBrandBg, kBrandInk);
        g.setColour(Palette::background);
        g.fillRect(juce::Rectangle<int>(0, kHeaderH, getWidth(), kContentH));

        if (!hero_.isEmpty()) {
            g.setColour(kBrandBg);
            g.fillRect(0, kHeaderH, getWidth(), (int) hero_.getBottom() + 12 - kHeaderH);
            g.setColour(kBrandInk.withAlpha(0.18f));
            g.drawLine(0.0f, hero_.getBottom() + 12.0f, (float) getWidth(),
                       hero_.getBottom() + 12.0f, 1.0f);
            drawHumusLogo(g, hero_);
        }
        drawHumusLogo(g, juce::Rectangle<float>(24.0f, 8.0f, 110.0f, (float) kHeaderH - 16.0f));
        g.setColour(kBrandInk);
        g.setFont(juce::FontOptions(20.0f, juce::Font::bold));
        g.drawText(tr("guide.meet-humus", "Meet Humus"), 150, 0, 240, kHeaderH, juce::Justification::centredLeft);
        g.setColour(kBrandInk.withAlpha(0.65f));
        g.setFont(juce::FontOptions(13.0f));
        g.drawText(juce::String::fromUTF8(guide::pageTitle(page_).c_str()), 0, 0,
                   getWidth() - 24, kHeaderH,
                   juce::Justification::centredRight);

        const int n = guide::kNumPages, dot = 8, gap = 14;
        const int x0 = (getWidth() - (n * dot + (n - 1) * (gap - dot))) / 2;
        const float cy = (float) getHeight() - kFooterH * 0.5f;
        for (int i = 0; i < n; ++i) {
            g.setColour(i == (int) page_ ? Palette::accent
                                         : kBrandInk.withAlpha(i < (int) page_ ? 0.55f : 0.22f));
            g.fillEllipse((float) (x0 + i * gap), cy - dot * 0.5f, (float) dot, (float) dot);
        }
    }

    bool keyPressed(const juce::KeyPress& k) override {
        if (k.getKeyCode() == juce::KeyPress::escapeKey) { finish(); return true; }
        if (k.getKeyCode() == juce::KeyPress::returnKey) { nextBtn_.triggerClick(); return true; }
        return false;
    }

private:
    static constexpr int kW = 760, kHeaderH = 64, kFooterH = 54, kContentH = 470;
    static constexpr int kHeroH = 150;
    static inline const juce::Colour kBrandBg{0xfff4ecdc}, kBrandInk{0xff33402a};

    class CordLegend : public juce::Component {
        void paint(juce::Graphics& g) override {
            struct Row { juce::Colour c; bool dashed; const char* label; };
            const Row rows[] = {{Palette::cord, false, "audio"},
                                {Palette::midiCord(), true, "MIDI"},
                                {Palette::videoCord(), false, "video"}};
            const int w = getWidth() / 3;
            g.setFont(juce::FontOptions(11.5f));
            for (int i = 0; i < 3; ++i) {
                auto cell = juce::Rectangle<int>(i * w, 0, w, getHeight()).reduced(14, 6);
                auto lane = cell.removeFromTop(cell.getHeight() - 18).toFloat();
                juce::Path p;
                p.startNewSubPath(lane.getX(), lane.getY() + 4.0f);
                p.cubicTo(lane.getCentreX(), lane.getBottom() + 6.0f,
                          lane.getCentreX(), lane.getBottom() + 6.0f,
                          lane.getRight(), lane.getY() + 4.0f);
                g.setColour(rows[i].c);
                const juce::PathStrokeType stroke(2.4f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded);
                if (rows[i].dashed) {
                    const float dashes[] = {5.0f, 4.0f};
                    juce::Path dashed;
                    stroke.createDashedStroke(dashed, p, dashes, 2);
                    g.fillPath(dashed);
                } else {
                    g.strokePath(p, stroke);
                }
                g.setColour(Palette::textDim);
                g.drawText(rows[i].label, cell, juce::Justification::centred);
            }
        }
    };

    class PodSketch : public juce::Component {
        static void cordTo(juce::Graphics& g, juce::Point<float> a, juce::Point<float> b) {
            juce::Path p;
            p.startNewSubPath(a);
            const float sag = 4.0f + std::abs(b.x - a.x) * 0.15f;
            p.cubicTo(a.x, a.y + sag, b.x, b.y - sag, b.x, b.y);
            g.setColour(Palette::cord);
            g.strokePath(p, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
        }
        static void node(juce::Graphics& g, juce::Rectangle<float> r,
                         const char* name, bool passThrough) {
            g.setColour(passThrough ? Palette::panel : Palette::panelLight);
            g.fillRoundedRectangle(r, 4.0f);
            g.setColour(passThrough ? Palette::accent.withAlpha(0.55f) : Palette::border);
            g.drawRoundedRectangle(r.reduced(0.5f), 4.0f, 1.0f);
            g.setColour(passThrough ? Palette::textDim : Palette::text);
            g.setFont(juce::FontOptions(10.0f));
            g.drawText(name, r.toNearestInt(), juce::Justification::centred);
            g.setColour(Palette::cord);
            for (const float y : {r.getY(), r.getBottom()})
                g.fillEllipse(r.getCentreX() - 2.5f, y - 2.5f, 5.0f, 5.0f);
        }

        void paint(juce::Graphics& g) override {
            const auto area = getLocalBounds().toFloat();
            const float cx = area.getCentreX();
            juce::Rectangle<float> pod(cx - 110.0f, 22.0f, 220.0f, area.getHeight() - 44.0f);

            cordTo(g, {cx, area.getY()}, {cx, pod.getY()});
            cordTo(g, {cx, pod.getBottom()}, {cx, area.getBottom()});

            g.setColour(Palette::panel.withAlpha(0.55f));
            g.fillRoundedRectangle(pod, 8.0f);
            g.setColour(Palette::accent);
            g.drawRoundedRectangle(pod.reduced(0.75f), 8.0f, 1.5f);
            g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
            g.setColour(Palette::textDim);
            g.drawText(tr("guide.myrig", "MyRig"), pod.toNearestInt().reduced(8, 3),
                       juce::Justification::topRight);
            g.setColour(Palette::cord);
            for (const float y : {pod.getY(), pod.getBottom()})
                g.fillEllipse(cx - 2.5f, y - 2.5f, 5.0f, 5.0f);

            const juce::Rectangle<float> inlet(cx - 34.0f, pod.getY() + 14.0f, 68.0f, 16.0f);
            const juce::Rectangle<float> outlet(cx - 37.0f, pod.getBottom() - 30.0f, 74.0f, 16.0f);
            const juce::Rectangle<float> left(cx - 88.0f, pod.getCentreY() - 11.0f, 72.0f, 22.0f);
            const juce::Rectangle<float> right(cx + 16.0f, pod.getCentreY() - 11.0f, 72.0f, 22.0f);
            cordTo(g, {inlet.getCentreX(), inlet.getBottom()}, {left.getCentreX(), left.getY()});
            cordTo(g, {inlet.getCentreX(), inlet.getBottom()}, {right.getCentreX(), right.getY()});
            cordTo(g, {left.getCentreX(), left.getBottom()}, {outlet.getCentreX(), outlet.getY()});
            cordTo(g, {right.getCentreX(), right.getBottom()}, {outlet.getCentreX(), outlet.getY()});
            node(g, inlet, "PodIn", true);
            node(g, outlet, "PodOut", true);
            node(g, left, "Rhizome", false);
            node(g, right, "Echo", false);
        }
    };

    void finish() {
        AppSettings::instance().set("guide.seen", 1);
        if (actions_.onFinished) actions_.onFinished();
    }

    void pickDemo() {
        const auto files = demoPatches();
        if (files.isEmpty()) return;
        juce::PopupMenu m;
        for (int i = 0; i < files.size(); ++i)
            m.addItem(i + 1, files[i].getFileNameWithoutExtension());
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&demoBtn_),
                        [this, files](int r) {
                            if (r <= 0) return;
                            const juce::File f = files[r - 1];
                            if (actions_.onOpenDemo) actions_.onOpenDemo(f);
                            finish();
                        });
    }

    Actions actions_;
    const bool firstBoot_;
    const bool hasDemos_ = !demoPatches().isEmpty();
    guide::Page page_ = guide::kWelcome;
    juce::Rectangle<float> hero_;

    juce::Viewport viewport_;
    std::unique_ptr<GuideBody> body_;
    CordLegend legend_;
    PodSketch podSketch_;
    juce::TextButton backBtn_{"Back"}, nextBtn_{"Next"}, skipBtn_;
    juce::TextButton demoBtn_{tr("guide.load-a-demo-patch", "Load a Demo Patch...")}, wizardBtn_{tr("guide.run-the-setup-wizard", "Run the Setup Wizard")};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GuideView)
};

}
