#include "core/BuiltinPacks.h"

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
