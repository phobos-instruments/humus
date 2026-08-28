#include "gui/VideoLayer.h"

#if !JUCE_MAC
namespace hum {
std::unique_ptr<VideoLayer> VideoLayer::create() { return nullptr; }
}
#endif
