# Tuning

Where a patch stops being in twelve notes. Everything that turns a note into a frequency asks this what the answer is. It makes no sound and sends no notes - it is a fact you patch.

## Reaching things

Cord it to an instrument and that instrument plays in this tuning. The tuning flows downstream, so Tuning into DNA into Rhizome tunes both. Two Tuning organisms put two synths in two different tunings at once. Cord it to nothing and it tunes the whole patch: that is the ordinary case, and the only way to reach an organism with no MIDI inlet, such as the autotuner Trellis, which is audio in and audio out.

## Parameters

**Preset** Equal temperaments first: 12-TET is the world's default; 19-EDO and 31-EDO are the old meantone-ish grids (19 has a sweeter minor third than 12); 24-EDO is quarter tones; Bohlen-Pierce repeats at a 3:1 tritave and contains no octave at all. Then just intonation, built from exact ratios with nothing tempered anywhere: 5-limit (the classic just chromatic), 7-limit (adds the barbershop sevenths), 17-limit (adds 17/16, 17/12 and the 13/8 neutral sixth, intervals twelve tempered tones cannot say), Harmonics 8-16 (one octave of the overtone series itself), and Pythagorean (everything from pure fifths). Last, Scala file loads any .scl - the format the microtonal world speaks, with thousands of scales published in it.

**Map** How a note number finds its pitch. Degrees is the raw index: note 45 is 24 degrees below the root, wherever that lands, so changing tuning transposes the patch. Keyboard spans one period across a normal 12-key octave and sounds the degree nearest each key's own position - a 7-note just scale lands on the white keys, 19-EDO plays its nearest-to-familiar twelve, and the octave key is always a true octave (or tritave). This is what makes any scale playable from ordinary hardware, and what keeps ordinary note numbers meaning ordinary registers.

**Divisions** Equal steps per octave for the Custom EDO preset, 5 to 64. Greyed out for the others, which set it themselves.

**Root** The MIDI note pinned to Root Hz. 69 is A4.

**Root Hz** What that note sounds at. 440 is standard, 432 the other one people ask for.

**File** The .scl file, for the Scala preset. Browse opens the scale library: every .scl in the scales folder beside your packs folder - drop in files, folders or symlinks - searchable by name, region, tradition or author, since the archive's own descriptions carry all of it. Picking a scale also selects this preset. A missing or malformed file plays as plain 12-TET rather than as garbage.

**Plugins** How hosted plugins are told. See below.

The piano roll still draws twelve rows per octave and labels every C. Under Map: Degrees in a non-12 scale those labels are lying to you, though the notes themselves are correct. Under Map: Keyboard the picture is honest again.

## Telling a plugin

MIDI cannot be asked whether it understands tuning messages, so on Auto - the default - the host runs the experiment instead. The first time this tuning meets a plugin, a background process loads it, retunes A4 over MTS, plays it and measures what came out. If the plugin speaks MTS it gets MTS from then on; if not, it gets pitch bend. The verdict is remembered next to the plugin scan data and re-checked when the plugin updates. Until it lands, bend is used, which works almost everywhere.

MTS SysEx sends real-time single-note tuning messages: exact and stateless, for synths implementing the MIDI Tuning Standard. Pitch Bend rewrites every note onto its own channel with a bend for the last 50 cents, which works with anything honouring per-channel bend at the ordinary two-semitone range, sandboxed plugins included. It costs polyphony - 16 voices - needs the plugin's bend range left at two semitones, and means you must not also bend that plugin from a controller, because the tuning owns its bend wheel now.

A plugin that understands neither simply stays in 12-TET; nothing breaks. The native instruments - Rhizome, Substrate, Mineral, Acid, Microdot, Trellis, DNA - always follow, no standard needed.

Try: just 17-limit, Map Keyboard, and hold a fifth. It locks beatless in a way no tempered fifth can. Harmonics 8-16 into a Rhizome is the overtone series playing itself.

## Related Organisms

Rhizome, Trellis, Substrate
