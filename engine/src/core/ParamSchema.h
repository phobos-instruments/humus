#pragma once
#include <string>
#include <vector>

namespace hum {

struct ParamDesc {
    std::string name;
    double min = 0.0;
    double max = 1.0;
    double def = 0.0;
    bool isBool = false;
    bool isEnum = false;
    bool isInt = false;
    bool isRange = false;
    double defMax = 0.0;
    std::string text;
    bool isText = false;
    bool isPlainText = false;
    bool isTrigger = false;
    bool randomize = false;
    bool defRandom = false;
    std::string unit;
};

const std::vector<ParamDesc>& schemaFor(const std::string& className);

}
