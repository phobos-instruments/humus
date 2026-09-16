# Leafcutter

A loop chopper that cuts a sound file at its transients and plays the slices in time with the transport at their original pitch.

Load a loop and it is cut at its transients; the slices then ride the transport tempo with no time-stretching, so a break stays clean at any tempo. Faster tempos truncate each slice, slower ones leave air between the cuts. Files in the sliced-loop formats load with their own slice map, tempo and beat count. The MIDI inlet plays slices like a kit, C3 for the first slice and each key up for the next, so a PianoRoll or a keyboard can finger the loop with or without the self-playing loop underneath. Cord the outlet into a Filter or a Fern.

## The map

The map shows the loop with a marker at every cut and lights the slice that is playing. Drag up or down on a slice to retune it on its own, and right-click it to reverse it, mute it or reset it, or to clear every slice edit. Those edits live in SliceEdits and stay with the patch.

## Parameters

**File** The loop to chop. Any sound file, or a sliced-loop file with its own slice map.

**Play** The self-playing loop. Off leaves only the MIDI kit.

**Sense** How many of the detected transients become cuts. Detection happens once at load, so the knob re-slices instantly.

**Gate** How much of each slice window sounds, from choppy to full.

**Pitch** Transposes every slice in semitones, -24 to 24, by resampling.

**Reverse** Plays every slice backwards.

**Decay** Fades each slice out from its start. 0 leaves slices whole; at the top every slice is a short hit.

**Shuffle** Slice order seed. 0 plays the loop as written; any other number is one repeatable rearrangement.

**Beats** Loop length in beats. 0 takes it from the file, or guesses from the length and the tempo.

**Level** Output level.

**SliceEdits** The per-slice pitch, reverse and mute edits made on the map, stored with the patch.

## Recipe

**Rechopped break** Load a two-bar drum loop, Beats 8, Sense 0.6, Gate 0.7, Shuffle 17, Decay 0.2. Press play and the break plays rearranged in time; roll the dice for another order and cord the outlet into a Filter with LfoSync on.

## Related Organisms

Sampler, Deck, PianoRoll, Repeater
