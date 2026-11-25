// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <string_view>

#include "core/packs/ClassString.h"
#include "core/packs/Roles.h"
#include "io/NewOrganism.h"
#include "io/PatchDocument.h"
#include "io/PatchWriter.h"

namespace hum {

inline std::string starterPatchText() {
    PatchDocumentModel starter;
    starter.version = "1";
    auto place = [&](std::string_view role, int x, int y) {
        const auto cls = classWithRole(role);
        if (cls.empty()) return std::string();
        const auto name = freeOrganismName(starter, parseClassString(cls).display);
        starter.organisms.push_back(newOrganismModel(cls, name, {}));
        OrganismView v;
        v.organismName = name;
        v.patcherX = x;
        v.patcherY = y;
        starter.views.push_back(v);
        return name;
    };
    const auto in = place(role::kAudioIn, 40, 40);
    place(role::kMidiIn, 220, 40);
    const auto out = place(role::kAudioOut, 40, 200);
    for (int ch = 0; ch < 2 && !in.empty() && !out.empty(); ++ch) {
        ConnectionModel c;
        c.src = in;
        c.srcOutlet = ch;
        c.dst = out;
        c.dstInlet = ch;
        starter.connections.push_back(c);
    }
    return writePatchString(starter);
}

}
