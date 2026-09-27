// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <set>
#include <string>
#include <vector>

namespace hum::rollscope {

inline constexpr const char* kWholeOrganism = "*";
inline constexpr const char* kPodActionPrefix = "Random:";
inline constexpr char kPodSep = '/';

inline bool isProtected(const std::set<std::string>& rollLocked) {
    return rollLocked.count(kWholeOrganism) > 0;
}

inline bool inPod(const std::string& organism, const std::string& pod) {
    return !pod.empty() && organism.size() > pod.size() + 1 && organism.compare(0, pod.size(), pod) == 0
           && organism[pod.size()] == kPodSep;
}

struct Candidate {
    std::string name;
    bool rolls = false;
    bool guarded = false;
};

inline std::vector<std::string> targets(const std::vector<Candidate>& all, const std::string& pod = {}) {
    std::vector<std::string> out;
    for (const auto& c : all)
        if (c.rolls && !c.guarded && (pod.empty() || inPod(c.name, pod))) out.push_back(c.name);
    return out;
}

inline std::vector<std::string> members(const std::vector<Candidate>& all, const std::string& pod) {
    std::vector<std::string> out;
    for (const auto& c : all)
        if (c.rolls && inPod(c.name, pod)) out.push_back(c.name);
    return out;
}

inline bool allGuarded(const std::vector<Candidate>& all, const std::string& pod) {
    bool any = false;
    for (const auto& c : all) {
        if (!c.rolls || !inPod(c.name, pod)) continue;
        if (!c.guarded) return false;
        any = true;
    }
    return any;
}

inline std::string podAction(const std::string& pod) { return std::string(kPodActionPrefix) + pod; }

inline bool isPodAction(const std::string& param) {
    const std::string prefix = kPodActionPrefix;
    return param.size() > prefix.size() && param.compare(0, prefix.size(), prefix) == 0;
}

inline std::string podOfAction(const std::string& param) {
    return isPodAction(param) ? param.substr(std::string(kPodActionPrefix).size()) : std::string();
}

inline std::string actionAfterRename(const std::string& param, const std::string& oldPod,
                                     const std::string& newPod) {
    const auto pod = podOfAction(param);
    if (pod == oldPod) return podAction(newPod);
    if (inPod(pod, oldPod)) return podAction(newPod + pod.substr(oldPod.size()));
    return param;
}

}
