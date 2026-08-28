# Phaser

The sweep. A chain of all-pass filters leaves the sound's level alone and moves only its phase, and when that phase-shifted copy is added back to the original, the places where the two disagree cancel into notches. Move the filters and the notches travel. It is the sound of a sweep passing through something rather than something being swept.

Use it on electric piano, on a pad that needs slow motion, or on a whole submix where a flanger would be too metallic: it is the subtler of the two sweeps, and the one that survives being left on.

## The family

Chorus, flanger and phaser are one idea at three scales. Chorus delays far enough to detune. Flanger delays briefly enough to comb, producing many evenly spaced notches. Phaser does not delay at all - it shifts phase - so its notches are few and unevenly spaced. That is the whole audible difference: a flanger is harmonically related to itself and sounds metallic, a phaser is not and sounds vocal.

## Parameters

**FrequencyRange** the two ends of the sweep. Both handles matter as much as the rate: a narrow range is a slow tonal wobble, a wide one is the full swoosh from bottom to top.

**Rate** how fast the notches travel between those ends.

**Feedback** returns the output to the input, which sharpens the notches into resonant peaks. This is where a phaser stops being polite.

**Depth** how much of the phase-shifted signal is mixed with the dry one. At zero there is nothing to interfere and the effect vanishes; the deepest notches are around the middle, where the two halves are evenly matched.

## How it works

Each all-pass stage passes every frequency at full level but delays the phase of some more than others, and the stages are cascaded so their shifts accumulate. Where the accumulated shift reaches half a cycle, the copy is exactly out of step with the original and the two cancel. The LFO moves the stages' break frequency, so those cancellation points travel together.

Because nothing is delayed in time, a phaser leaves transients intact - a drum hit still arrives when it arrived. That is why it can sit on a full mix without smearing it.

## Related Organisms

Flanger, Chorus, Filter
