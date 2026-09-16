# Tuning

The patch's tuning: every organism that turns a note into a frequency asks this what the answer is.

It makes no sound and sends no notes. Equal temperaments, just intonation from exact ratios, the overtone series and any .scl scale file are on one preset list, and Map decides how ordinary note numbers find their pitch in a scale without twelve steps. The native instruments, Rhizome, Substrate, Wave, pH, Mineral, Acid, Microdot and Trellis, follow it directly; hosted plugins are told over MIDI as the Plugins row describes. The piano roll still draws twelve rows per octave, so under Map Degrees in a non-12 scale its labels no longer match the pitches, though the notes are correct.

## Reaching organisms

Cord its MIDI outlet to an instrument and that instrument plays in this tuning. The tuning flows on down the MIDI cords, so Tuning into DNA into Rhizome tunes both, and two Tuning organisms put two synths in two tunings at once. Cord it to nothing and it tunes the whole patch, which is the ordinary case and the only way to reach an organism with no MIDI inlet, such as Trellis.

## Parameters

**Preset** The scale. 12-TET (standard), 19-EDO, 24-EDO (quarter tones), 31-EDO and Bohlen-Pierce, which repeats at a 3:1 tritave with no octave at all; Custom EDO with its own Divisions; Just - 5-limit, Just - 7-limit and Just - 17-limit built from exact ratios; Harmonics 8-16, one octave of the overtone series; Pythagorean, everything from pure fifths; and Scala file (.scl).

**Divisions** Equal steps per octave for Custom EDO, 5 to 64. Dimmed for the other presets, which set it themselves.

**Root** The MIDI note pinned to RootHz. 69 is A4.

**RootHz** What that note sounds at. 440 is standard.

**Map** How a note number finds its pitch. Degrees (1:1) is the raw index, so changing the scale transposes the patch. Keyboard spans one period across a normal twelve-key octave and sounds the degree nearest each key's own position, so a seven-note just scale lands on the white keys and the octave key stays a true octave.

**File** The .scl file for the Scala file preset. Browse opens the scale library beside your packs folder; picking a scale also selects the preset. A missing file plays as plain 12-TET.

**Plugins** How hosted plugins are told. MTS SysEx sends single-note tuning messages to plugins that understand the standard. Pitch bend puts each note on its own channel with a bend for the remainder, which works almost everywhere but limits polyphony and takes over the bend wheel. Auto (probe) tests each plugin once and remembers the verdict.

## Recipe

**Beatless fifth** Preset Just - 17-limit, Map Keyboard, cord it to nothing so the whole patch follows. Hold a fifth on a Rhizome from a PianoRoll: it locks without beating in a way no tempered fifth can. Then switch to Harmonics 8-16 and play a scale on the same Rhizome for the overtone series itself.

## Related Organisms

Rhizome, Trellis, Substrate
