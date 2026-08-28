#pragma once
#include <vector>

namespace hum {

class Registry;

struct BuiltinPack {
    const char* id;
    void (*registerFactories)(Registry&);
};

const std::vector<BuiltinPack>& builtinPacks();

}
