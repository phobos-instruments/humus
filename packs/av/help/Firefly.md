# Firefly

A flash over the picture, on the beat, by hand or from notes.

One video inlet and one outlet: whatever comes in passes through, and the flash is laid over it. With nothing corded in the flash lands on black, so Firefly on its own is a lamp. It fires on the transport's grid, at a free rate of its own, from the Flash button, or from notes arriving at its MIDI inlet, so a MIDI Track, DNA or Riff on the timeline drives it like anything else. Note velocity sets how strong that flash is. Hue and Saturation give it a colour, and because both are ordinary params an LFO on Hue sweeps the colour while you play. Each flash is timed by the picture's own clock, so a flash shorter than a frame still shows. A warning worth heeding: fast flashing can trigger seizures in people with photosensitive epilepsy. Keep it slow in front of an audience you do not know, and say it is coming.

## Parameters

**Mode** What fires it on its own. Beat fires on the transport's grid and stays with the song, resting while the transport is stopped. Free runs at its own Rate whether the song plays or not. Played never fires by itself: only the Flash button and notes at the MIDI inlet do, so the lights follow your hands or a MIDI Track and nothing else. The button and notes fire in every mode.

**Division** How often Beat mode fires against the beat: a whole note through a thirty-second, plus triplets and dotted values.

**Rate** Flashes per second in Free mode, from a slow pulse at 0.05 Hz to 30 Hz.

**Width** How long each flash lasts, as a share of the gap between flashes; in Played mode the gap is one beat at the song's tempo. Short is a camera pop; wide spends as much time lit as dark.

**Hue** The colour of the flash in degrees around the wheel. It only shows when Saturation is up.

**Saturation** How coloured the flash is. 0% is white, 100% is the full hue.

**Level** How strongly the flash covers the picture. 100% replaces it for the length of the flash.

**Flash** Fires one flash by hand. Map it to a pad or a footswitch and play the lights.

## Recipe

**Beat flash over a video** Cord a VideoPlayer into Firefly and Firefly into a VideoOut. Mode Beat, Division 1/4, Width 12%, Shape 40%, Saturation 0%. Press play and the picture pops on every beat.

**Played from the timeline** Set Mode to Played, cord a MIDI Track into Firefly's MIDI inlet and write a line of notes. Loud notes flash hard, quiet ones barely show, so the lights read the part you wrote.

## Related Organisms

VideoFX, Lumen, VideoOut
