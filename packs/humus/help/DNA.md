# DNA

A generative sequencer that plays a genome. Sixteen bases (A/C/G/T) loop at Rate, locked to the transport; each base sounds its interval over Root - the Chord preset's voicing, or your own (see Bases below). Wire its MIDI outlet into any instrument: Acid, Mineral, Substrate, a plugin.

## Mutation

Every bar there is a Mutate-probability chance that one base flips - DNA replication with copying errors - so the line slowly evolves as it plays. Seed regrows a fresh strand deterministically: the same Seed always grows the same starting genome.

## Parameters

**Root / Chord** Key and colour. Nine four-note voicings: Minor, Major, Phrygian, Pentatonic, Dorian, Harmonic Minor, Whole Tone, Hirajoshi, and Harmonic (JI) - the 4:5:6:7 series in just cents, which blooms microtonal under a JI tuning.

**Rate** A rotary switch, 1/64 buzz-rolls to 1/1 chord tones: 1/16, 1/8, 1/32, triplets (1/8T, 1/16T, 1/4T), dotted (1/8., 1/16.), 1/4, 1/2, 1/1, 1/64.

**Gate** Note length as a fraction of the step.

**Rest** Thins the strand into a groove. Rests spread statistically and each Seed breathes its own way; turning Rest up only ever adds rests to the same line.

**Octaves** Lets bases express an octave up for range.

**Mutate** The evolution speed. 0 = a fixed riff; high = restless.

While the transport plays, the light strip above the base rows shows where in the sixteen-step strand the sequence has arrived: the lit cell is the playing step, and recessed cells are the rests of the current pass - the groove, visible.

## Silencing steps

Rest decides where the gaps fall for you. To put one exactly where you want it, click a cell on the strip: it goes flat and grey and that step never sounds again, whatever Rest or the mutation are doing. Click it again to bring it back, or drag along the strip to sweep several. The steps that survive keep their places - silencing one never shifts the groove of the others. Right-click the strip for the whole strand at once: play every step, invert, or silence the off-beats.

## Bases

The four key rows at the bottom show each base's interval over Root. By default every base follows the Chord preset (dim key). Click a key to re-voice just that base - its own interval, 0..12 semitones (bright key) - and click the lit key again to follow the preset once more. Presets and overrides mix freely: keep Minor and lift only T to the 11th, or build a voicing from scratch.

Try: DNA -> Acid for an evolving acid line; a second DNA (different Seed, 1/8T, Phrygian) -> Mineral for triplet percussion shuffling over it.

## Related Organisms

Mineral, Substrate, Rhizome
