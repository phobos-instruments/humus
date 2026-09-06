#pragma once
#include <vector>

namespace hum {

bool bodyCoreMLReady();

bool bodyCoreMLPredict(bool landmarkNet, const float* in, int count,
                       std::vector<float>& out0, std::vector<float>& out1);

}
