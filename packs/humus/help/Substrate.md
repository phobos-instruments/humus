# Substrate

The layer everything grows on: an evolving drone/pad of stacked strata. Up to 64 detuned saw layers climb an Interval ladder above the root, each with its own slow, independent drift in pitch and level - the bed never sits still. Layers alternate across the stereo field. The preset rail carries the classic ladders (Octaves, Fifths, Thirds) and a few whole beds - recall one, then bend it.

## Parameters

**Drone** On: sounds continuously at Note. Off: plays from MIDI (Attack/Release). Drone is always one root - it overrides Mode.

**Mode** How MIDI notes become voices. Mono: one root, last note wins. Para: one envelope and one filter, but the layer pool is dealt across every held note - hold a triad with 12 layers and each note grows a 4-high stack; release a note and its strata flow back to what you still hold. Costs the same no matter how many keys are down. Poly: every note gets the full stack with its own attack/release, eight voices.

**Interval** How many scale degrees each rung climbs, asked of the active tuning. In ordinary 12-TET, degrees are semitones - 12 stacks octaves, 7 fifths, 4 thirds, and anything between is a bed no fixed ladder offers. Under a Tuning node the same number walks that scale's own ladder, like Rhizome's Interval does. The ladder wraps after eight rungs, so tall Layer counts pile on as drifting unison copies instead of climbing away.

**Layers** How many strata (1-64). Past ~16 it stops being a chord of layers and becomes a texture - 64 is a supersaw bed.

**Spread** Drift depth in cents - how far the strata wander apart.

**Motion** How much the drift moves pitch and level. 0 = frozen.

**Cutoff** The bed's lowpass. Automate it for the classic pad swell.

Try: Drone on, the Fifths preset, Motion high, Cutoff low - then ride Cutoff from the Metapad while DNA and Acid play over the top. Or Drone off, Para, Layers 24, and hold slow chords from a keyboard: the strata re-deal themselves to every change of harmony.

## Related Organisms

Tuning, Rhizome, DNA
