// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/packs/BuiltinPacks.h"

namespace hum {

void hum_register_pack_core(Registry&);
void hum_register_pack_humus(Registry&);
void hum_register_pack_av(Registry&);

const std::vector<BuiltinPack>& builtinPacks() {
    static const std::vector<BuiltinPack> packs = {
        {"core", &hum_register_pack_core},
        {"humus", &hum_register_pack_humus},
        {"av", &hum_register_pack_av},
    };
    return packs;
}

}
