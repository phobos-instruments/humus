#pragma once
#include <cmath>
#include <functional>
#include <memory>
#include <thread>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/PluginHost.h"
#include "core/PluginListStore.h"
#include "core/PluginScanner.h"
#include "gui/AppSettings.h"
#include "gui/StartWindow.h"
#include "gui/Telemetry.h"
#include "gui/TelemetryEvents.h"

namespace hum {

class SplashWindow : public juce::Component, private juce::Timer {
public:
    explicit SplashWindow(std::function<void()> onDone) : onDone_(std::move(onDone)) {
        PluginHost::instance().restoreKnownListFromXml(pluginListStore::load());
        pruneMissing();
        baseCount_ = PluginHost::instance().knownPlugins().getNumTypes();

        scanner_ = std::make_unique<PluginScanner>(PluginHost::instance());
        scanner_->setSubprocessExe(
            juce::File::getSpecialLocation(juce::File::currentExecutableFile));
        juce::StringArray extra;
        extra.addLines(AppSettings::instance().getString("plugins.searchPaths"));
        extra.removeEmptyStrings(true);
        scanner_->start(extra, {});
        if (AppSettings::instance().getInt("plugins.skipVersion", 1) < 2) {
            juce::StringArray keep, cur;
            cur.addLines(AppSettings::instance().getString("plugins.scanSkip", ""));
            for (const auto& e : cur)
                if (!e.containsChar('|')) keep.add(e);
            AppSettings::instance().set("plugins.scanSkip", keep.joinIntoString("\n"));
            AppSettings::instance().set("plugins.skipVersion", 2);
        }
        juce::StringArray skip;
        skip.addLines(AppSettings::instance().getString("plugins.scanSkip", ""));
        scanner_->skipKnownFiles(skip);
        total_ = (int) scanner_->remaining();

        if (total_ == 0) finishScan();
        else {
            status_ = "Found " + juce::String(total_) + " new plugin file"
                      + (total_ == 1 ? "" : "s") + juce::String::fromUTF8("\xe2\x80\xa6");
            scanner_->beginAsync(juce::jlimit(
                2, 6, (int) std::thread::hardware_concurrency() / 2));
        }

        shownAt_ = juce::Time::getMillisecondCounter();
        setSize(460, 320);
        startTimerHz(30);
    }

    ~SplashWindow() override { stopTimer(); }

    void paint(juce::Graphics& g) override {
        const juce::Colour bg(0xfff4ecdc), line(0xff33402a), tipCol(0xff7e8f3c);
        paintBrandPanel(g, getLocalBounds(), bg, line);
        drawHumusLogo(g, juce::Rectangle<float>(0.0f, 18.0f, (float) getWidth(), 184.0f));
        g.setColour(line.withAlpha(0.6f));
        g.setFont(juce::Font(juce::FontOptions(12.0f)));
        juce::String ver;
        if (auto* app = juce::JUCEApplication::getInstance())
            ver << "v" << app->getApplicationVersion();
        g.drawText(ver, 0, 210, getWidth(), 18, juce::Justification::centred);

        g.setColour(line.withAlpha(0.75f));
        g.setFont(juce::Font(juce::FontOptions(12.0f)));
        g.drawText(status_, 24, 248, getWidth() - 48, 18,
                   juce::Justification::centred, true);

        if (scanner_) {
            const auto bar = juce::Rectangle<float>(
                60.0f, 278.0f, (float) getWidth() - 120.0f, 4.0f);
            g.setColour(line.withAlpha(0.15f));
            g.fillRoundedRectangle(bar, 2.0f);
            g.setColour(tipCol);
            g.fillRoundedRectangle(bar.withWidth(juce::jmax(4.0f, bar.getWidth()
                                                 * scanner_->progress())), 2.0f);
            g.setColour(line.withAlpha(0.5f));
            g.setFont(juce::Font(juce::FontOptions(11.0f)));
            g.drawText("skip", skipBounds(), juce::Justification::centred);
        }
    }

    void mouseMove(const juce::MouseEvent& e) override {
        setMouseCursor(scanner_ && skipBounds().contains(e.getPosition())
                           ? juce::MouseCursor::PointingHandCursor
                           : juce::MouseCursor::NormalCursor);
    }
    void mouseUp(const juce::MouseEvent& e) override {
        if (scanner_ && skipBounds().contains(e.getPosition())) finishScan();
    }

private:
    static constexpr int kMinShowMs = 1200;

    juce::Rectangle<int> skipBounds() const {
        return {getWidth() - 64, getHeight() - 28, 44, 20};
    }

    static void pruneMissing() {
        auto& kp = PluginHost::instance().knownPlugins();
        for (const auto& d : kp.getTypes())
            if (d.fileOrIdentifier.startsWithChar('/')
                && !juce::File(d.fileOrIdentifier).exists())
                kp.removeType(d);
    }

    void finishScan() {
        if (scanner_) {
            scanner_->cancelAsync();
            persistProgress();
            if (!scanner_->failed().isEmpty())
                telemetryCount(telemetry::kPluginScanFailed,
                               scanner_->failed().size());
            scanner_.reset();
        }
        const int now = PluginHost::instance().knownPlugins().getNumTypes();
        const int added = now - baseCount_;
        status_ = added > 0
            ? juce::String(added) + " new plugin" + (added == 1 ? "" : "s") + " added"
            : juce::String(now) + " plugins ready";
        repaint();
    }

    void persistProgress() {
        pluginListStore::save(PluginHost::instance().knownListToXml());
        juce::StringArray skip;
        skip.addLines(AppSettings::instance().getString("plugins.scanSkip", ""));
        for (const auto& f : scanner_->failed()) skip.addIfNotAlreadyThere(f);
        skip.removeEmptyStrings();
        AppSettings::instance().set("plugins.scanSkip", skip.joinIntoString("\n"));
        lastSavedCount_ = PluginHost::instance().knownPlugins().getNumTypes();
    }

    void timerCallback() override {
        if (scanner_) {
            juce::String cur;
            const bool more = scanner_->pumpAsync(cur);
            done_ = total_ - (int) scanner_->remaining();
            auto name = cur.fromLastOccurrenceOf("/", false, false);
            if (name.containsChar('.')) name = name.upToLastOccurrenceOf(".", false, false);
            const int shownDone = juce::jlimit(
                0, total_, (int) std::lround(scanner_->progress() * (float) total_));
            status_ = "Scanning "  + name + "  (" + juce::String(shownDone)
                    + "/" + juce::String(total_) + ")";
            const int now = PluginHost::instance().knownPlugins().getNumTypes();
            if (now - lastSavedCount_ >= 12) persistProgress();
            repaint();
            if (more) return;
            finishScan();
        }
        if (juce::Time::getMillisecondCounter() - shownAt_ >= (juce::uint32) kMinShowMs) {
            stopTimer();
            if (onDone_) {
                auto f = std::move(onDone_);
                onDone_ = nullptr;
                f();
            }
        }
    }

    std::function<void()> onDone_;
    std::unique_ptr<PluginScanner> scanner_;
    int lastSavedCount_ = 0;
    juce::String status_{juce::CharPointer_UTF8("Checking for new plugins\xe2\x80\xa6")};
    int baseCount_ = 0, total_ = 0, done_ = 0;
    juce::uint32 shownAt_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SplashWindow)
};

}
