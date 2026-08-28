#pragma once
#include <thread>
#include <functional>
#include <memory>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/AppPaths.h"
#include "core/ClassString.h"
#include "core/PluginHost.h"
#include "core/PluginListStore.h"
#include "core/PluginScanner.h"
#include "gui/AppSettings.h"
#include "gui/LookAndFeel.h"
#include "gui/PluginBridgePolicy.h"
#include "gui/PluginQuarantine.h"

namespace hum {

class PluginManagerView : public juce::Component,
                          private juce::ListBoxModel,
                          private juce::Timer {
public:
    std::function<void()> onPluginsChanged;

    PluginManagerView() {
        pathsLabel_.setText(juce::String("Extra search paths (one per line - format defaults always scanned):"),
                            juce::dontSendNotification);
        pathsLabel_.setColour(juce::Label::textColourId, Palette::textDim);
        pathsLabel_.setFont(juce::FontOptions(11.5f));
        addAndMakeVisible(pathsLabel_);

        paths_.setMultiLine(true);
        paths_.setReturnKeyStartsNewLine(true);
        paths_.setText(AppSettings::instance().getString("plugins.searchPaths"),
                       juce::dontSendNotification);
        addAndMakeVisible(paths_);

        scanBtn_.setButtonText("Scan for plugins");
        scanBtn_.onClick = [this] { startScan(); };
        addAndMakeVisible(scanBtn_);

        status_.setColour(juce::Label::textColourId, Palette::textDim);
        status_.setFont(juce::FontOptions(11.0f));
        addAndMakeVisible(status_);

        sandbox_.setColour(juce::ToggleButton::textColourId, Palette::text);
        sandbox_.setToggleState(bridgePolicy::sandboxEnabled(), juce::dontSendNotification);
#if JUCE_LINUX
        sandbox_.setButtonText("Run plugins in separate processes (recommended)");
        sandbox_.onClick = [this] {
            bridgePolicy::setSandboxEnabled(sandbox_.getToggleState());
            list_.updateContent();
        };
#else
        sandbox_.setButtonText("Run plugins in separate processes (Linux only)");
        sandbox_.setEnabled(false);
#endif
        addAndMakeVisible(sandbox_);

        list_.setModel(this);
        list_.setRowHeight(24);
        list_.setColour(juce::ListBox::backgroundColourId, Palette::panel);
        addAndMakeVisible(list_);
        rebuildRows();

        quarantineBtn_.onClick = [this] { quarantine::clear(); refreshQuarantineBtn(); };
        addAndMakeVisible(quarantineBtn_);
        refreshQuarantineBtn();

        setSize(560, 460);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(12);
        pathsLabel_.setBounds(r.removeFromTop(18));
        paths_.setBounds(r.removeFromTop(64));
        r.removeFromTop(8);
        auto row = r.removeFromTop(26);
        scanBtn_.setBounds(row.removeFromLeft(140));
        row.removeFromLeft(10);
        quarantineBtn_.setBounds(row.removeFromRight(190));
        row.removeFromRight(10);
        status_.setBounds(row);
        r.removeFromTop(8);
        sandbox_.setBounds(r.removeFromTop(24));
        r.removeFromTop(6);
        list_.setBounds(r);
    }

private:
    struct Row : juce::Component {
        juce::ToggleButton toggle{"separate process"};
        juce::String label;
        Row() {
            toggle.setColour(juce::ToggleButton::textColourId, Palette::textDim);
            addAndMakeVisible(toggle);
        }
        void resized() override { toggle.setBounds(getWidth() - 150, 0, 148, getHeight()); }
        void paint(juce::Graphics& g) override {
            g.setColour(Palette::text);
            g.setFont(juce::FontOptions(13.0f));
            g.drawText(label, 6, 0, getWidth() - 156, getHeight(),
                       juce::Justification::centredLeft, true);
        }
    };

    int getNumRows() override { return (int) rows_.size(); }
    void paintListBoxItem(int, juce::Graphics&, int, int, bool) override {}

    juce::Component* refreshComponentForRow(int row, bool, juce::Component* existing) override {
        std::unique_ptr<Row> r(dynamic_cast<Row*>(existing));
        if (row < 0 || row >= (int) rows_.size()) { return nullptr; }
        if (r == nullptr) r = std::make_unique<Row>();
        const auto& d = rows_[(size_t) row];
        r->label = d.name + "  -  " + d.pluginFormatName + (d.isInstrument ? "  (instrument)" : "");
        const auto raw = PluginHost::classRawFor(d);
        const bool master = bridgePolicy::sandboxEnabled();
        r->toggle.setEnabled(master);
        r->toggle.setToggleState(master && !bridgePolicy::isInProcess(raw), juce::dontSendNotification);
        r->toggle.onClick = [raw, p = &r->toggle] {
            bridgePolicy::setInProcess(raw, !p->getToggleState());
        };
        r->repaint();
        return r.release();
    }

    void startScan() {
        AppSettings::instance().set("plugins.searchPaths", paths_.getText());
        juce::StringArray extra;
        extra.addLines(paths_.getText());
        extra.removeEmptyStrings(true);
        const auto pedal = appDataDir().getChildFile("plugin-scan-pedal.txt");
        pedal.getParentDirectory().createDirectory();
        scanner_ = std::make_unique<PluginScanner>(PluginHost::instance());
        scanner_->setSubprocessExe(juce::File::getSpecialLocation(juce::File::currentExecutableFile));
        scanner_->start(extra, pedal);
        scanner_->beginAsync(juce::jlimit(2, 6, (int) std::thread::hardware_concurrency() / 2));
        scanBtn_.setEnabled(false);
        startTimerHz(30);
    }

    void timerCallback() override {
        if (!scanner_) { stopTimer(); return; }
        juce::String current;
        if (scanner_->pumpAsync(current)) {
            status_.setText("Scanning: " + current, juce::dontSendNotification);
            return;
        }
        stopTimer();
        scanBtn_.setEnabled(true);
        const auto failedCount = scanner_->failed().size();
        pluginListStore::save(PluginHost::instance().knownListToXml());
        AppSettings::instance().set("plugins.scanSkip",
                                    scanner_->failed().joinIntoString("\n"));
        status_.setText(juce::String("Scan complete - ") +
                            juce::String(PluginHost::instance().knownPlugins().getNumTypes())
                            + " plugins" + (failedCount > 0
                                ? " (" + juce::String(failedCount) + " failed)" : juce::String()),
                        juce::dontSendNotification);
        rebuildRows();
        scanner_.reset();
        if (onPluginsChanged) onPluginsChanged();
    }

    void rebuildRows() {
        rows_ = PluginHost::instance().knownPlugins().getTypes();
        list_.updateContent();
        list_.repaint();
    }

    void refreshQuarantineBtn() {
        const int n = quarantine::list().size();
        quarantineBtn_.setButtonText("Clear UI quarantine (" + juce::String(n) + ")");
        quarantineBtn_.setVisible(n > 0);
    }

    juce::Label pathsLabel_, status_;
    juce::TextEditor paths_;
    juce::TextButton scanBtn_, quarantineBtn_;
    juce::ToggleButton sandbox_;
    juce::ListBox list_;
    juce::Array<juce::PluginDescription> rows_;
    std::unique_ptr<PluginScanner> scanner_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginManagerView)
};

}
