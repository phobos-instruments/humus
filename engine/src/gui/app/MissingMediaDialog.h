// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/host/EngineHost.h"
#include "gui/common/Localisation.h"
#include "io/MediaRefs.h"

namespace hum {

namespace missingmedia {

inline juce::String describe(const media::Ref& r) {
    juce::String where(juce::CharPointer_UTF8(r.organism.c_str()));
    if (r.isClip()) where << " " << tr("missing-media.clip", "clip");
    else where << " " << juce::String(juce::CharPointer_UTF8(r.param.c_str()));
    return where + ":  " + r.leaf();
}

inline juce::String summary(const std::vector<media::Ref>& missing) {
    juce::String s;
    const int shown = std::min((int) missing.size(), 8);
    for (int i = 0; i < shown; ++i) s << describe(missing[(size_t) i]) << "\n";
    if ((int) missing.size() > shown)
        s << tr("missing-media.and", "and ") << juce::String((int) missing.size() - shown)
          << tr("missing-media.more", " more") << "\n";
    return s;
}

inline juce::String statusLine(int count) {
    return juce::String(count)
           + (count == 1 ? tr("missing-media.one-missing", " media file is missing - File > Locate Missing Media")
                         : tr("missing-media.many-missing", " media files are missing - File > Locate Missing Media"));
}

inline void show(EngineHost& host, std::function<void()> onChanged) {
    const auto missing = host.missingMedia();
    if (missing.empty()) return;
    auto* w = new juce::AlertWindow(
        tr("missing-media.title", "Missing media"),
        tr("missing-media.body",
           "These files were not found where the patch expects them. Locate one and every "
           "file that moved with it is found too; Ignore keeps the patch as it is.")
            + "\n\n" + summary(missing),
        juce::MessageBoxIconType::WarningIcon);
    w->addButton(tr("missing-media.locate", "Locate..."), 1, juce::KeyPress(juce::KeyPress::returnKey));
    w->addButton(tr("missing-media.ignore", "Ignore"), 0, juce::KeyPress(juce::KeyPress::escapeKey));
    w->enterModalState(true, juce::ModalCallbackFunction::create([&host, onChanged, first = missing.front(), w](int r) {
        std::unique_ptr<juce::AlertWindow> owned(w);
        if (r != 1) return;
        const juce::File start = first.resolved != juce::File()
                                     && first.resolved.getParentDirectory().isDirectory()
                                     ? first.resolved.getParentDirectory()
                                     : juce::File::getSpecialLocation(juce::File::userHomeDirectory);
        auto chooser = std::make_shared<juce::FileChooser>(
            tr("missing-media.locate-title", "Locate ") + first.leaf(), start.getChildFile(first.leaf()), "*");
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                             [&host, onChanged, first, chooser](const juce::FileChooser& fc) {
            const auto f = fc.getResult();
            if (f == juce::File()) return;
            host.relocateMedia(first, f);
            if (onChanged) onChanged();
            juce::MessageManager::callAsync([&host, onChanged] { show(host, onChanged); });
        });
    }), false);
}

}

}
