# Tuning

The patch's tuning: every organism that turns a note into a frequency asks this what the answer is.

It makes no sound and sends no notes. Equal temperaments, just intonation from exact ratios, the overtone series and any .scl scale file are all in one scale browser, and Map decides how ordinary note numbers find their pitch in a scale without twelve steps. The native instruments, Rhizome, Substrate, Wave, pH, Mineral, Acid, Microdot and Trellis, follow it directly; hosted plugins are told over MIDI as the Plugins row describes. The piano roll still draws twelve rows per octave, so under Map Every step in a non-12 scale its labels no longer match the pitches, though the notes are correct.

## Reaching organisms

Cord its MIDI outlet to an instrument and that instrument plays in this tuning. The tuning flows on down the MIDI cords, so Tuning into DNA into Rhizome tunes both, and two Tuning organisms put two synths in two tunings at once. Cord it to nothing and it tunes the whole patch, which is the ordinary case and the only way to reach an organism with no MIDI inlet, such as Trellis.

## Parameters

**Preset** The mode, at the bottom of the box. Scale file, the default, plays the scale chosen in the browser at the top, and a new Tuning starts there on 12-TET, ordinary tuning, which is a scale file like any other. Custom EDO divides the octave into its own number of equal Divisions and dims the browser, keeping its scale for when you come back. The scales this list used to hold - 12-TET, 19, 24 and 31-EDO, Bohlen-Pierce, the 5, 7 and 17-limit just scales, Harmonics 8-16 and Pythagorean - are in the browser now, note for note the same. A patch saved on one of them still opens on it, shows its name here and plays as it always did.

**Divisions** Equal steps per octave for Custom EDO, 1 to 128. One is octaves only, two adds the tritone, and 128 is the most a scale can hold. Dimmed in Scale file mode, where the file decides.

**Root** The MIDI note pinned to RootHz. 69 is A4.

**RootHz** What that note sounds at. 440 is standard.

**Map** How a key finds its note when the scale does not have twelve of them. Every step plays the next note of the scale on each key, so every note is reachable, a 19-note octave takes 19 keys, and changing the scale moves everything. Fit to 12 keys spreads one period of the scale over an ordinary twelve-key octave and sounds the note nearest each key's usual place, so a seven-note just scale lands on the white keys and the octave key stays a true octave; with more than twelve notes some have no key. With a twelve-note scale the two are the same.

**File** The scale, at the top of the box: the .scl file Scale file mode plays. Click the name to open the scale browser: the scales that ship with the app, your own, the ones you brought in from elsewhere and the ones you used lately, each with its note count and description, and a search that reads all of it. The arrows step to the next and previous scale. Picking a scale also selects the preset. A missing file is marked in amber and plays as plain 12-TET.

**Plugins** How hosted plugins are told. MTS SysEx sends single-note tuning messages to plugins that understand the standard. Pitch bend puts each note on its own channel with a bend for the remainder, which works almost everywhere but limits polyphony and takes over the bend wheel. Auto (probe) tests each plugin once and remembers the verdict.

## Recipe

**Beatless fifth** Preset Just - 17-limit, Map Keyboard, cord it to nothing so the whole patch follows. Hold a fifth on a Rhizome from a PianoRoll: it locks without beating in a way no tempered fifth can. Then switch to Harmonics 8-16 and play a scale on the same Rhizome for the overtone series itself.

## Related Organisms

Rhizome, Trellis, Substrate
