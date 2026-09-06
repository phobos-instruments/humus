#pragma once
#include <string>
#include <vector>

namespace hum {

inline constexpr int kMp3Quality = 0;

bool writeMp3(const std::string& path,
              const std::vector<std::vector<float>>& channels,
              double sampleRate);

double mp3BytesPerSecond();

}
