# Leafcutter

The loop chopper, named for the ant that cuts leaves into pieces to feed the garden. Load a loop and it is cut at its transients; the slices then ride the transport tempo at their original pitch, in the sliced-loop tradition - no time-stretching, so a break stays clean at any tempo. Faster tempos truncate each slice, slower ones leave air between the cuts.

## Chopping

Sense decides how many of the detected transients become cuts: low keeps only the strongest hits, high cuts at every flutter. Detection happens once when the file loads, so the knob re-slices instantly and is safe to automate. Gate shortens every slice inside its window - pull it down for choppy, gated rhythms. Shuffle re-chops the order: 0 plays the loop as written, any other number is a repeatable rearrangement, so the same seed always renders the same groove. The dice rolls all three.

## Time

The loop spans a whole number of beats. Beats 0 guesses it from the file length at the current tempo; set it by hand when the guess is wrong or the loop should sprawl. Rolling, the chop rides the transport and jumps with it on a seek; stopped, the clock free-runs from where it was, so the groove keeps going while you patch.

## Playing it

Play on, the loop chops by itself. The MIDI inlet plays slices like a kit - C3 is the first slice, each key up the next one - so a PianoRoll or a keyboard can finger the loop, with or without the self-playing loop underneath. Velocity sets the slice's level.

## Sliced-loop files

Files in the classic sliced-loop formats (.rx2, .rex) load with their own slice map, tempo and beat count - the cuts land exactly where the loop's maker put them, and Beats fills itself in. Older files squeeze their audio with a codec that was never publicly documented; for those, place a sound-file bounce with the same name beside the file (loop.rx2 next to loop.wav) and the original slice map is laid over that audio.

## Parameters

**File** the loop to chop. Any sound file, or a sliced-loop file.

**Play** the self-playing loop. Off leaves only the MIDI kit.

**Sense** how many transients become cuts, 0 to 1.

**Gate** how much of each slice window sounds, choppy to full.

**Shuffle** slice order seed. 0 = as written; each number is one rearrangement.

**Beats** loop length in beats. 0 guesses from the file and tempo.

**Level** output level.

## Related Organisms

Sampler, Deck, PianoRoll, Drops, Repeater
