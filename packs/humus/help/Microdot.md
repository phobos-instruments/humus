# Microdot

The rolling offbeat bass, boxed. A 16-step trigger grid locked to the transport fires the hits; every hit is a tight band-limited saw (plus an optional sub-octave square) through a fast filter envelope and drive. A fresh box comes seeded with the classic psy roll - three 16ths after each kick - so it locks in the moment you press Play. Silent while stopped. While it plays, the column the transport is on wears a light, so you can see which box is firing.

## The grid

Click a step to toggle it. Two thin rows live below: the ^ row lifts that step's hit one octave (the classic jumping line), and the hold row at the bottom is sustain, not accent - a lit socket stretches the previous note through that step, and a run of them holds one note across all of theirs. Right-click for the stamps - Roll -111, Offbeat --1-, Full 1111, Random, Clear - then draw your own on top. The pattern is stored in the patch.

## Played from outside

The grid is always the gate; Seq says who owns the pitch. With Seq on, every hit plays the Note field. Flip Seq off and the hits follow whatever note the MIDI inlet is holding (following the patch Tuning) - cord a DNA, Riff, Steps or PianoRoll in and its line gets re-chopped by your boxes, and when nothing is held the grid stays quiet. Hold a chord and the last note pressed wins. While the transport is stopped a note-on plays one hit on its own, so a keyboard still lets you audition the voice.

## Parameters

**Note** The pitch every hit plays while Seq is on.

**Wave** Six voices: saw, square, thin pulse, triangle, pure sine (all fundamental - let Sub and Drive talk), and a detuned saw pair for width.

**Seq** Who owns the pitch: on = the Note field, off = the held MIDI note. The grid gates either way.

**Cutoff / Reso / EnvMod / Decay** the filter pluck. Reso is the squelch: raise it and the EnvMod sweep starts to sing. Decay shapes the note as well as the sweep - short is a pluck that dies, long holds through the gate. Keep Decay short (80-150 ms) and EnvMod moderate for the tight rolling character.

**Gate** Hit length as a fraction of the step - shorter = bouncier.

**Sub** Sub-octave square underneath, for weight.

**Drive** Saturation - push for harder styles.

Pair with a Kick in 4/4 mode: the two interlock by construction.

## Related Organisms

Kick, Acid, Riff, DNA
