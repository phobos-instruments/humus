#pragma once
#include <cmath>

namespace hum {

struct BlepOsc {
    double phase = 0.0;

    void reset(double p = 0.0) { phase = p; }

    static double blep(double t, double dt) {
        if (t < dt) { t /= dt; return t + t - t * t - 1.0; }
        if (t > 1.0 - dt) { t = (t - 1.0) / dt; return t * t + t + t + 1.0; }
        return 0.0;
    }

    float saw(double dt) {
        const double v = 2.0 * phase - 1.0 - blep(phase, dt);
        step(dt);
        return (float) v;
    }

    float square(double dt) {
        double v = (phase < 0.5 ? 1.0 : -1.0) + blep(phase, dt);
        double t2 = phase + 0.5;
        if (t2 >= 1.0) t2 -= 1.0;
        v -= blep(t2, dt);
        step(dt);
        return (float) v;
    }

    float pulse(double dt, double width) {
        width = width < 0.05 ? 0.05 : width > 0.95 ? 0.95 : width;
        double v = (phase < width ? 1.0 : -1.0) + blep(phase, dt);
        double t2 = phase + (1.0 - width);
        if (t2 >= 1.0) t2 -= 1.0;
        v -= blep(t2, dt);
        step(dt);
        return (float) (v - (2.0 * width - 1.0));
    }

    float tri(double dt) {
        const double v = phase < 0.5 ? 4.0 * phase - 1.0 : 3.0 - 4.0 * phase;
        step(dt);
        return (float) v;
    }

    float sine(double dt) {
        const double v = std::sin(6.283185307179586 * phase);
        step(dt);
        return (float) v;
    }

private:
    void step(double dt) {
        phase += dt;
        if (phase >= 1.0) phase -= 1.0;
    }
};

}
