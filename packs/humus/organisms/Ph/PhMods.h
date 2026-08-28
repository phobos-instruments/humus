#pragma once

namespace hum {

struct PhMods {
    double bright = 0.5;
    double attack = 0.5;
    double release = 0.5;
    double detune = 0.0;
    double vibrato = 0.0;
    double speed = 0.5;

    bool operator==(const PhMods& o) const {
        return bright == o.bright && attack == o.attack && release == o.release
               && detune == o.detune && vibrato == o.vibrato && speed == o.speed;
    }
    bool operator!=(const PhMods& o) const { return !(*this == o); }
};

}
