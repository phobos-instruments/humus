// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <functional>

namespace hum::momentary {

inline constexpr int kConfirmTicks = 36;
inline constexpr int kTicksPerSecond = 60;

inline int coverTicksFor(double sampleRate, int bufferSize) {
    double ms = 25.0;
    if (sampleRate > 0.0) ms = std::max(25.0, 2000.0 * bufferSize / sampleRate);
    return std::max(2, (int) std::ceil(ms / 16.7));
}

class Press {
public:
    Press(bool holdable, bool confirmHold) : holdable_(holdable), confirmHold_(confirmHold) {}

    std::function<void(double)> write;
    std::function<int()> coverTicks;

    bool firesOnClick() const { return !holdable_ && !confirmHold_; }
    bool confirmHold() const { return confirmHold_; }
    bool confirming() const { return confirming_; }
    double confirmFraction() const { return (double) heldTicks_ / (double) kConfirmTicks; }
    bool running() const { return running_; }
    bool takeRestart() {
        const bool was = restart_;
        restart_ = false;
        return was;
    }

    void click() {
        if (up_) {
            send(0.0);
            up_ = false;
            rearm_ = cover();
        } else {
            send(1.0);
            up_ = true;
            frames_ = cover();
            rearm_ = 0;
        }
        start();
    }

    void down(bool menuGesture) {
        if (confirmHold_ && !menuGesture) {
            heldTicks_ = 0;
            confirming_ = true;
            start();
            return;
        }
        if (!holdable_ || menuGesture) return;
        send(1.0);
        up_ = true;
        frames_ = cover();
        release_ = false;
        start();
    }

    void up(bool menuGesture) {
        if (confirming_) {
            confirming_ = false;
            heldTicks_ = 0;
            if (frames_ == 0 && rearm_ == 0) running_ = false;
            return;
        }
        if (!holdable_ || menuGesture || !up_) return;
        release_ = true;
    }

    void tick() {
        if (confirming_) {
            if (++heldTicks_ >= kConfirmTicks) {
                confirming_ = false;
                heldTicks_ = 0;
                click();
            }
            return;
        }
        if (holdable_) {
            if (frames_ > 0) --frames_;
            if (release_ && frames_ == 0) {
                send(0.0);
                up_ = false;
                release_ = false;
                running_ = false;
            }
            return;
        }
        if (rearm_ > 0) {
            if (--rearm_ == 0) {
                send(1.0);
                up_ = true;
                frames_ = cover();
            }
            return;
        }
        if (frames_ > 0 && --frames_ == 0) {
            send(0.0);
            up_ = false;
            running_ = false;
        }
    }

private:
    void start() {
        running_ = true;
        restart_ = true;
    }
    void send(double value) { if (write) write(value); }
    int cover() const { return coverTicks ? coverTicks() : coverTicksFor(0.0, 0); }

    bool holdable_ = false, confirmHold_ = false;
    bool confirming_ = false;
    int heldTicks_ = 0;
    bool up_ = false, release_ = false;
    int frames_ = 0, rearm_ = 0;
    bool running_ = false, restart_ = false;
};

}
