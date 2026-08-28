# VCA

The multiplier: output = signal x control x Gain. The left inlet is the signal, the right inlet the control; an unconnected control reads as unity, so a bare VCA is just a gain.

This is the glue that makes control signals do something:

- LFO -> control inlet: tremolo.

- Follower -> control inlet: gate/expander (or ducking, inverted via a Number chain).

- Audio -> control inlet with Bipolar on: ring modulation.

Without Bipolar the control is clamped at zero so ordinary unipolar control signals can't flip the signal's phase; Bipolar passes negatives through for audio-rate modulation.

## Parameters

**Gain** a fixed multiplier on top of the control inlet. With nothing patched there this is the whole story, and the VCA is simply a gain.

**Bipolar** lets the control go negative, which is what makes audio-rate modulation possible. Off it is clamped at zero, so a unipolar control can never flip the phase of what passes through.

## Related Organisms

Gain, LFO, Follower
