#pragma once
#include <functional>
#include <string>

namespace hum::followpick {

using Pick = std::function<void(const std::string& organism, const std::string& param)>;

inline Pick& pick() { static Pick f; return f; }
inline bool armed() { return (bool) pick(); }
inline void arm(Pick f) { pick() = std::move(f); }
inline void cancel() { pick() = nullptr; }

inline bool take(const std::string& organism, const std::string& param) {
    if (!armed() || organism.empty() || param.empty()) return false;
    auto f = pick();
    cancel();
    f(organism, param);
    return true;
}

}
