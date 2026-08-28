#pragma once
#include <string>
#include <vector>

#include "core/ModControl.h"

namespace hum {

class EngineHost;

inline constexpr const char* kParamSource = "param:";
inline bool isParamSource(const std::string& value) {
    return value.rfind(kParamSource, 0) == 0;
}
inline std::string paramSourceName(const std::string& value) {
    return isParamSource(value) ? value.substr(std::string(kParamSource).size()) : value;
}

class ModHost {
public:
    explicit ModHost(EngineHost& host) : host_(host) {}

    void mapRoute(const std::string& source, const std::string& value,
                  const std::string& organism, const std::string& param,
                  double min, double max);
    void clearRoute(const std::string& source, const std::string& value,
                    const std::string& organism, const std::string& param);
    void clearForOrganism(const std::string& organism);
    void renameOrganism(const std::string& oldName, const std::string& newName);
    const ModControlMap& map() const { return map_; }
    void setShape(const std::string& source, const std::string& value,
                  const std::string& organism, const std::string& param,
                  const ControlShape& shape);

    std::vector<ModParamUpdate> tick(double dt);

    std::vector<std::pair<std::string, std::string>> availableSources() const;

    void syncMapFromModel();
    void syncMapToModel();

private:
    EngineHost& host_;
    ModControlMap map_;
};

}
