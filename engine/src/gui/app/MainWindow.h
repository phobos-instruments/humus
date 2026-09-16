// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/app/WindowGeometry.h"
#include "gui/app/AppSettings.h"
#include "gui/app/MainComponent.h"

namespace hum {

class MainWindow : public juce::DocumentWindow {
public:
    static constexpr int kMinW = 900, kMinH = 600;

    MainWindow()
        : juce::DocumentWindow(juce::String("Untitled  -  Humus"),
                               juce::Colours::black,
                               juce::DocumentWindow::allButtons) {
        setUsingNativeTitleBar(true);
        setContentOwned(new MainComponent(), true);
        setResizable(true, true);
        setResizeLimits(kMinW, kMinH, 100000, 100000);
        const auto state = AppSettings::instance().getString("window.state");
        if (!savedWindowSizeIsUsable(state.toStdString(), kMinW, kMinH)
            || !restoreWindowStateFromString(state)) {
            const auto area = juce::Desktop::getInstance().getDisplays()
                                  .getPrimaryDisplay()->userArea;
            const auto s = firstRunWindowSize(area.getWidth(), area.getHeight(),
                                              kMinW, kMinH);
            centreWithSize(s.w, s.h);
        }
        setAlpha(0.0f);
        setVisible(true);
#if JUCE_LINUX || JUCE_WINDOWS
        if (auto* peer = getPeer())
            peer->setIcon(juce::ImageCache::getFromMemory(
                BinaryData::icon1024_png, BinaryData::icon1024_pngSize));
#endif
#if JUCE_LINUX
        if (auto* peer = getPeer()) {
            peer->setConstrainer(getConstrainer());
            if (getWidth() < kMinW || getHeight() < kMinH)
                setSize(juce::jmax(kMinW, getWidth()), juce::jmax(kMinH, getHeight()));
        }
#endif
    }

    void closeButtonPressed() override {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }
};

}
