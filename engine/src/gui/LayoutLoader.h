#pragma once
#include <string>

#include "hum/LayoutSpec.h"

namespace hum {

LayoutSpec loadLayoutSpec(const std::string& jsonText);
LayoutSpec loadLayoutSpecFromFile(const std::string& path);

bool knownLayoutControlType(const std::string& type);

}
