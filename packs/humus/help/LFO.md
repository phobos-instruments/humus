# LFO

A low-frequency oscillator as a patchable object: emits Offset + Amplitude x wave at Rate Hz from its outlet, added to whatever arrives at its inlet (Numbers and LFOs chain by summing - an LFO can wobble around a Number's value).

Waveform picks sine, triangle or square. With Sync on, one cycle lasts SyncBeats beats of the transport instead of Rate Hz, so the wobble locks to the song's tempo; the Beats box steps through musical divisions, and holding + or - walks them.

The face draws one cycle of the wave it is emitting, carrying its Amplitude and Offset, with a playhead riding the cycle wherever the oscillator has got to. It is one cycle rather than a scrolling trace because an LFO runs faster than a screen can follow, and a picture of the shape stays readable where a trace would turn to noise. Sample-and-hold has no shape to draw, so it shows the level it is holding.

Right-click any parameter and Modulate with offers this LFO twice: "wave" is the wave itself, and "phase" is the rising ramp of the cycle - useful where you want a sweep locked to the LFO rather than the LFO's own shape.

Patch it into a VCA's control inlet for tremolo, into another LFO for drifting, never-repeating motion, or anywhere an audio inlet wants slow movement. All of its parameters are morphable from the Metapad - a corner with Rate 0.1 and a corner with Rate 20 makes the modulation speed itself a performance gesture.

## Parameters

**Rate** cycles per second, when Sync is off.

**Waveform** sine, triangle, square, saw, down-saw, or sample-and-hold. S&H steps to a new random value once per cycle instead of sweeping.

**Amplitude** how far the wave travels either side of Offset.

**Offset** the centre the wave moves around. Shift it to keep an otherwise bipolar wobble positive.

**Sync** locks one cycle to the transport instead of to Rate.

**SyncBeats** how many beats one cycle lasts while Sync is on. Below 1 for faster-than-beat motion. One beat is a quarter note, so 0.5 is an eighth and 4 is a bar of four-four.

## Related Organisms

VCA, Number, Follower
