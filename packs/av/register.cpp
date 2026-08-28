#include "hum/Registry.h"

#include "CameraIn/CameraIn.h"
#include "Lumen/Lumen.h"
#include "VideoPlayer/VideoPlayer.h"
#include "VideoMix/VideoMix.h"
#include "VideoOut/VideoOut.h"

namespace hum {

void hum_register_pack_av(Registry& r) {
    r.registerClass("CameraIn", [] { return std::make_unique<CameraIn>(); });
    r.registerClass("CamIn", [] { return std::make_unique<CameraIn>(); });
    r.registerClass("Lumen", [] { return std::make_unique<Lumen>(); });
    r.registerClass("VideoPlayer", [] { return std::make_unique<VideoPlayer>(); });
    r.registerClass("VHS", [] { return std::make_unique<VideoPlayer>(); });
    r.registerClass("VideoMix", [] { return std::make_unique<VideoMix>(); });
    r.registerClass("VideoOut", [] { return std::make_unique<VideoOut>(); });
}

}
