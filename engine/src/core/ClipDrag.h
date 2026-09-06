#pragma once
#include <cstdlib>
#include <string>

namespace hum::clipdrag {

inline constexpr const char* kVideoClip = "videoclip:";
inline constexpr const char* kVideoPad = "videopad:";

inline std::string tagged(const char* tag, const std::string& node, int number) {
    return std::string(tag) + node + ":" + std::to_string(number);
}

inline bool parseTagged(const char* tag, const std::string& text, std::string& node, int& number) {
    const std::string prefix(tag);
    if (text.rfind(prefix, 0) != 0) return false;
    const auto colon = text.rfind(':');
    if (colon == std::string::npos || colon < prefix.size() || colon + 1 >= text.size()) return false;
    node = text.substr(prefix.size(), colon - prefix.size());
    number = std::atoi(text.c_str() + colon + 1);
    return !node.empty();
}

inline std::string videoClip(const std::string& node, int clipId) {
    return tagged(kVideoClip, node, clipId);
}

inline bool parseVideoClip(const std::string& text, std::string& node, int& clipId) {
    return parseTagged(kVideoClip, text, node, clipId);
}

inline std::string videoPad(const std::string& node, int pad) {
    return tagged(kVideoPad, node, pad);
}

inline bool parseVideoPad(const std::string& text, std::string& node, int& pad) {
    return parseTagged(kVideoPad, text, node, pad);
}

}
