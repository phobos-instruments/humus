// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include <string>

#include "hum/Registry.h"

#include "CameraIn/CameraIn.h"
#include "Hands/Hands.h"
#include "Skeleton/Skeleton.h"
#include "Lumen/Lumen.h"
#include "VideoPlayer/VideoPlayer.h"
#include "VideoTrack/VideoTrack.h"
#include "VideoPad/VideoPad.h"
#include "Firefly/Firefly.h"
#include "VideoFX/VideoFX.h"
#include "VideoMix/VideoMix.h"
#include "VideoOut/VideoOut.h"

namespace hum {

void hum_register_layouts_av(Registry& r);

void hum_register_pack_av(Registry& r) {
    r.registerClass("CameraIn", [] { return std::make_unique<CameraIn>(); });
    r.registerClass("Hands", [] { return std::make_unique<Hands>(); });
    r.registerFlag("system-hands", [] { return handsSystemTracker(); });
    r.registerClass("Skeleton", [] { return std::make_unique<Skeleton>(); });
    r.registerClass("Lumen", [] { return std::make_unique<Lumen>(); });
    r.registerClass("VideoPlayer", [] { return std::make_unique<VideoPlayer>(); });
    r.registerClass("VideoPad", [] { return std::make_unique<VideoPad>(); });
    r.registerClass("VideoTrack", [] { return std::make_unique<VideoTrack>(); });
    r.registerClass("Firefly", [] { return std::make_unique<Firefly>(); });
    r.registerClass("VideoFX", [] { return std::make_unique<VideoFX>(); });
    r.registerClass("VideoMix", [] { return std::make_unique<VideoMix>(); });
    r.registerClass("VideoMix2", [] { return std::make_unique<VideoMix>(); });
    for (int n = 3; n <= kVideoMixMostChannels; ++n)
        r.registerClass("VideoMix" + std::to_string(n), [n] { return std::make_unique<VideoChannelMix>(n); });
    hum_register_layouts_av(r);
    r.registerClass("VideoOut", [] { return std::make_unique<VideoOut>(); });
}

}
