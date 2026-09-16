# Substrate

An evolving drone or pad of up to 64 detuned saw layers stacked on an interval ladder.

Each layer climbs one rung of Interval above the root and carries its own slow drift in pitch and level; layers alternate across the stereo field. Interval is asked of the patch tuning, so under a Tuning organism the ladder walks that scale's own degrees. The ladder wraps after eight rungs, so tall Layer counts pile on as drifting unison copies, and the level holds as layers are added. Presets carry the Octaves, Fifths and Thirds ladders. Play it from a PianoRoll or a DNA, or leave Drone on and ride Cutoff from a Metapad.

## Parameters

**Drone** Off plays from MIDI through Attack and Release. On sounds continuously at Note, always as one root, whatever Mode says.

**Note** The root while Drone is on, or while nothing has been played in Mono mode.

**Layers** How many strata, 1 to 64. Past about 16 it stops being a chord of layers and becomes a texture.

**Interval** How many scale degrees each rung climbs. In ordinary tuning 12 stacks octaves, 7 fifths and 4 thirds; anything between is a bed no fixed ladder offers.

**Spread** Drift depth in cents, how far the strata wander apart.

**Motion** How much the drift moves pitch and level. 0 freezes the bed.

**Cutoff** The bed's lowpass. Automate it for the pad swell.

**Attack** How long the bed takes to open, in milliseconds.

**Release** How long it takes to fade after the notes lift.

**Level** Output level.

**BendRange** How far the pitch wheel reaches at full travel, in semitones. Two is the common default; zero ignores the wheel.

**Mode** How MIDI notes become voices. Mono is one root, last note wins. Para deals the layer pool across every held note, so a triad with 12 layers gives each note a four-high stack, at the same cost however many keys are down. Poly gives every note the full stack with its own envelope, eight voices.

## Recipe

**Re-dealing pad** Drone off, Mode Para, Layers 24, Interval 7, Motion 0.6, Attack 1500, Release 3000, Cutoff 1200. Hold slow chords from a PianoRoll; the layers deal themselves across each held note and flow back to what is still held when a note lifts.

## Related Organisms

Tuning, Rhizome, DNA
