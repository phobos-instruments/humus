#pragma once
#include <string>
#include <vector>

namespace hum {

bool writeWav(const std::string& path,
              const std::vector<std::vector<float>>& channels,
              double sampleRate, int bitsPerSample = 24);

}
