#pragma once
#include <string>
#include <vector>

namespace hum {

struct InstalledPack {
    std::string id;
    std::string version;
};

struct PackUpdate {
    std::string id;
    std::string version;
    std::string url;
    std::string notes;
};

int compareVersions(const std::string& a, const std::string& b);

std::vector<PackUpdate> availableUpdates(const std::string& manifestJson,
                                         const std::vector<InstalledPack>& installed,
                                         int hostAbi,
                                         const std::string& platformTag);

}
