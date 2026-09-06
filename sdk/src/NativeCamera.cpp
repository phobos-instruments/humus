#include "hum/NativeCamera.h"

#include <cstdint>

#if !defined(__APPLE__)

namespace hum {

std::unique_ptr<NativeCamera> NativeCamera::open(
    const std::string&, std::function<void(const FrameRef&)>) {
    return nullptr;
}

bool NativeCamera::copyRgba(const FrameRef&, bool, std::vector<std::uint8_t>&, int&,
                            int&) {
    return false;
}

}

#endif
