#include "gui/GamepadHost.h"

#include <cmath>

#include "gui/EngineHost.h"

namespace hum {

namespace {
constexpr const char* kAxisName[GamepadSnapshot::kAxes] = {"lx", "ly", "rx", "ry", "lt", "rt"};
constexpr const char* kButtonName[GamepadSnapshot::kButtons] = {"a", "b", "x", "y"};
}

void GamepadHost::setEnabled(bool on) {
    enabled_ = on;
    last_.clear();
    names_.clear();
    if (on) startTimer(33);
    else stopTimer();
}

std::string GamepadHost::statusText() const {
    if (!enabled_) return "off";
    if (names_.empty()) return "none connected";
    std::string s;
    for (const auto& n : names_) s += (s.empty() ? "" : ", ") + n;
    return s;
}

void GamepadHost::timerCallback() {
    std::vector<GamepadSnapshot> pads;
    gamepadPlatformPoll(pads);
    if (pads.size() != last_.size()) last_.assign(pads.size(), Last{});
    names_.clear();
    for (size_t i = 0; i < pads.size(); ++i) {
        names_.push_back(pads[i].name);
        auto& prev = last_[i];
        const std::string base = "/joy/" + std::to_string(i + 1) + "/";
        for (int a = 0; a < GamepadSnapshot::kAxes; ++a) {
            const float v = pads[i].axes[a];
            if (prev.seeded && std::abs(v - prev.axes[a]) < 0.004f) continue;
            prev.axes[a] = v;
            if (prev.seeded) host_.osc().inject(base + kAxisName[a], v);
        }
        for (int b = 0; b < GamepadSnapshot::kButtons; ++b) {
            const float v = pads[i].buttons[b];
            if (prev.seeded && v == prev.buttons[b]) continue;
            prev.buttons[b] = v;
            if (prev.seeded) host_.osc().inject(base + kButtonName[b], v);
        }
        prev.seeded = true;
    }
}

}

#if defined(__linux__)

#include <fcntl.h>
#include <linux/joystick.h>
#include <unistd.h>

namespace hum {
namespace {
struct JsDev {
    int fd = -1;
    std::string path;
    GamepadSnapshot state;
};
std::vector<JsDev> jsDevs;
double jsLastScanMs = 0.0;

void jsScan() {
    for (int n = 0; n < 4; ++n) {
        const std::string path = "/dev/input/js" + std::to_string(n);
        bool open = false;
        for (auto& d : jsDevs) open |= d.path == path && d.fd >= 0;
        if (open) continue;
        const int fd = ::open(path.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;
        JsDev d;
        d.fd = fd;
        d.path = path;
        char label[128] = {};
        if (ioctl(fd, JSIOCGNAME(sizeof(label)), label) >= 0 && label[0])
            d.state.name = label;
        else
            d.state.name = path;
        jsDevs.push_back(std::move(d));
    }
}
}

void gamepadPlatformPoll(std::vector<GamepadSnapshot>& out) {
    out.clear();
    const double now = juce::Time::getMillisecondCounterHiRes();
    if (now - jsLastScanMs > 2000.0) { jsLastScanMs = now; jsScan(); }
    for (auto it = jsDevs.begin(); it != jsDevs.end();) {
        js_event e;
        bool dead = false;
        while (true) {
            const auto n = ::read(it->fd, &e, sizeof(e));
            if (n != (ssize_t) sizeof(e)) {
                dead = n < 0 && errno != EAGAIN && errno != EWOULDBLOCK;
                break;
            }
            const int type = e.type & ~JS_EVENT_INIT;
            const float v = (float) e.value / 32767.0f;
            auto& s = it->state;
            if (type == JS_EVENT_AXIS) {
                switch (e.number) {
                    case 0: s.axes[0] = (v + 1.0f) * 0.5f; break;
                    case 1: s.axes[1] = 1.0f - (v + 1.0f) * 0.5f; break;
                    case 2: s.axes[4] = (v + 1.0f) * 0.5f; break;
                    case 3: s.axes[2] = (v + 1.0f) * 0.5f; break;
                    case 4: s.axes[3] = 1.0f - (v + 1.0f) * 0.5f; break;
                    case 5: s.axes[5] = (v + 1.0f) * 0.5f; break;
                    default: break;
                }
            } else if (type == JS_EVENT_BUTTON && e.number < GamepadSnapshot::kButtons) {
                s.buttons[e.number] = e.value ? 1.0f : 0.0f;
            }
        }
        if (dead) {
            ::close(it->fd);
            it = jsDevs.erase(it);
            continue;
        }
        out.push_back(it->state);
        ++it;
    }
}

}

#elif !defined(__APPLE__)

namespace hum {
void gamepadPlatformPoll(std::vector<GamepadSnapshot>& out) { out.clear(); }
}

#endif
