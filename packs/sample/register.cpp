#include "hum/Registry.h"

#include "Tremolo/Tremolo.h"
#include "PingPong/PingPong.h"

namespace hum {

void hum_register_pack_sample(Registry& r) {
    r.registerClass("Tremolo",  [] { return std::make_unique<Tremolo>(); });
    r.registerClass("PingPong", [] { return std::make_unique<PingPong>(); });
}

}
