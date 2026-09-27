// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/browser/ProjectFacts.h"

#include <filesystem>
#include <system_error>

#include "core/browser/FileKind.h"
#include "core/packs/Categories.h"
#include "core/xml/Xml.h"
#include "hum/FileBytes.h"

namespace hum::browser {

namespace {

bool bookkeeping(const std::string& cls) {
    return cls.size() >= 8 && cls.compare(cls.size() - 8, 8, "PseudoSP") == 0;
}

char familyLetter(Family f) {
    switch (f) {
        case Family::Voice: return 'V';
        case Family::Time: return 'T';
        case Family::Motion: return 'M';
        case Family::Sense: return 'S';
        case Family::Utility: break;
    }
    return 'U';
}

}

std::string patchOf(const std::string& projectOrPatch) {
    std::error_code ec;
    const auto p = utf8Path(projectOrPatch);
    if (!std::filesystem::is_directory(p, ec)) return projectOrPatch;
    std::string first;
    for (const auto& item : std::filesystem::directory_iterator(p, ec)) {
        const auto path = utf8Text(item.path());
        if (kindOfFile(path) != Kind::Patch) continue;
        if (first.empty() || path < first) first = path;
    }
    return first;
}

Facts probeProject(const std::string& projectOrPatch) {
    Facts f;
    f.probed = true;
    const auto patch = patchOf(projectOrPatch);
    if (patch.empty()) return f;
    const auto doc = xml::parseFile(patch);
    const auto* body = doc != nullptr ? doc->child("patch") : nullptr;
    if (body == nullptr) return f;
    if (const auto* clock = body->child("clock")) f.bpm = clock->doubleAttribute("tempo", 0.0);
    for (const auto* c : body->children()) {
        if (c->tag() != "contraption") continue;
        const auto cls = c->attribute("class", "");
        if (cls.empty() || bookkeeping(cls)) continue;
        ++f.boxes;
        if ((int) f.families.size() < kFamiliesMost) f.families += familyLetter(familyOf(cls));
    }
    return f;
}

}
