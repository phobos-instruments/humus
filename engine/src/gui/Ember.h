#pragma once
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/UiTicker.h"

namespace hum {

namespace ember {

constexpr int kDecayMs = 500;

class Clock {
public:
    static Clock& instance() {
        static Clock c;
        return c;
    }

    void stamp(juce::Component& c) {
        const auto now = juce::Time::getMillisecondCounter();
        for (auto& e : embers_)
            if (e.comp.getComponent() == &c) { e.t0 = now; ensureTicking(); return; }
        embers_.push_back({juce::Component::SafePointer<juce::Component>(&c), now});
        ensureTicking();
    }

private:
    struct Ember {
        juce::Component::SafePointer<juce::Component> comp;
        juce::uint32 t0 = 0;
    };

    void ensureTicking() {
        if (tickerId_ == 0) tickerId_ = UiTicker::instance().add([this] { tick(); });
    }

    void tick() {
        const auto now = juce::Time::getMillisecondCounter();
        for (size_t i = embers_.size(); i-- > 0;) {
            auto& e = embers_[i];
            auto* c = e.comp.getComponent();
            if (c == nullptr) { embers_.erase(embers_.begin() + (long) i); continue; }
            const float heat = 1.0f - (float) (now - e.t0) / (float) kDecayMs;
            if (heat <= 0.0f) {
                c->getProperties().remove("ember");
                c->repaint();
                embers_.erase(embers_.begin() + (long) i);
                continue;
            }
            c->getProperties().set("ember", (double) heat);
            c->repaint();
        }
        if (embers_.empty() && tickerId_ != 0) {
            UiTicker::instance().remove(tickerId_);
            tickerId_ = 0;
        }
    }

    std::vector<Ember> embers_;
    int tickerId_ = 0;
};

inline void stamp(juce::Component& c) { Clock::instance().stamp(c); }

}
}
