// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/app/BundleCheck.h"

#include <string_view>

#include "core/app/AppPaths.h"
#include "core/library/ExamplesTree.h"
#include "core/packs/PackRegistry.h"
#include "hum/Registry.h"

namespace hum::bundlecheck {

namespace {
constexpr std::string_view kLayoutPrefix = "layout:";
}

Report run() {
    registerBuiltinOrganisms();
    Report report;
    auto note = [&report](bool ok, const std::string& what) {
        report.lines.push_back((ok ? "ok    " : "FAIL  ") + what);
        if (!ok) ++report.problems;
    };
    const auto packs = PackRegistry::instance().builtinRoot();
    note(!packs.empty(), "packs: " + (packs.empty() ? std::string("not found") : packs));
    const auto assets = bundledAssetsDir();
    note(assets != juce::File(), "assets: " + (assets == juce::File() ? std::string("not found") : pathOf(assets)));
    const auto shipped = examples::roots();
    note(!shipped.empty(), "examples: " + (shipped.empty() ? std::string("not found") : pathOf(shipped.front())));
    int faces = 0, found = 0;
    for (const auto& cls : Registry::instance().classNames()) {
        const auto* manifest = PackRegistry::instance().classManifest(cls);
        if (manifest == nullptr || manifest->editor.rfind(kLayoutPrefix.data(), 0, kLayoutPrefix.size()) != 0) continue;
        ++faces;
        const auto* folder = PackRegistry::instance().folderOf(cls);
        const auto file = folder != nullptr ? fileAt(folder->dir + "/" + manifest->editor.substr(kLayoutPrefix.size())) : juce::File();
        if (file.existsAsFile()) ++found;
        else note(false, "face file for " + cls + ": " + manifest->editor);
    }
    note(faces > 0 && found == faces, "faces: " + std::to_string(found) + " of " + std::to_string(faces) + " found");
    return report;
}

}
