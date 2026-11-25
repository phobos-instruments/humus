// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <juce_events/juce_events.h>

namespace hum {

template <class T>
class ShutdownSingleton : private juce::DeletedAtShutdown {
public:
    static T& instance() {
        if (held() == nullptr) held() = new T();
        return *held();
    }

protected:
    ShutdownSingleton() = default;
    ~ShutdownSingleton() override { held() = nullptr; }

private:
    static T*& held() {
        static T* made = nullptr;
        return made;
    }
};

}
