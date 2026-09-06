#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace hum {

class NativeCamera {
public:
    struct FrameRef {
        void* buffer = nullptr;
        int width = 0, height = 0;
        std::shared_ptr<const void> hold;
    };

    virtual ~NativeCamera() = default;

    static std::unique_ptr<NativeCamera> open(
        const std::string& deviceName,
        std::function<void(const FrameRef&)> onFrame);

    static bool copyRgba(const FrameRef& frame, bool mirror,
                         std::vector<std::uint8_t>& rgba, int& width, int& height);

protected:
    NativeCamera() = default;
};

}
