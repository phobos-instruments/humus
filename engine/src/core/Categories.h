#pragma once
#include <string>
#include <utility>
#include <vector>

namespace hum {

std::string categoryOf(const std::string& className);

bool isHiddenOrganism(const std::string& className);

std::string pseudoOwnerLabel(const std::string& className);

std::string canonicalClass(const std::string& className);

std::pair<std::string, std::string> splitCategory(const std::string& category);

std::vector<std::string> categorySegments(const std::string& category);

std::string originOf(const std::string& className);

enum class Family { Voice, Time, Motion, Sense, Utility };
Family familyOf(const std::string& className);

using CategoryGroups = std::vector<std::pair<std::string, std::vector<std::string>>>;

CategoryGroups groupByCategory(const std::vector<std::string>& classNames);

std::vector<std::pair<std::string, CategoryGroups>>
groupByOrigin(const std::vector<std::string>& classNames);

}
