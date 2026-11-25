# DNA

A generative sequencer that plays a sixteen-base genome as MIDI notes.

Sixteen bases, A, C, G and T, loop at Rate against the transport, and each base sounds its interval over Root: the Chord voicing's, or one of your own set on the base keys at the bottom of the editor. Every bar there is a Mutate chance that one base flips, so the line slowly evolves as it plays, while the same Seed always grows the same starting strand. The strip above the base rows shows the playing step and the rests of the current pass; click a cell to silence that step outright, drag to sweep several, right-click for the whole strand. Cord the MIDI outlet into an Acid, a Mineral or a Substrate.

**Step chance** Cmd-drag a cell of the strip up or down (Ctrl-drag on Windows and Linux) to set how often that step plays, from never to every time in steps of 5%, on top of Rest. A step below 100% draws fainter and shows its chance. Right-click a cell for quick chances, the strip's own actions, and MIDI Learn for that step's chance. The dice belong to the box: a patch plays the same steps on every play and every export.

## Parameters

**Root** The key, as a MIDI note.

**Chord** The voicing the four bases take: Minor, Major, Phrygian, Pentatonic, Dorian, Harmonic Minor, Whole Tone, Hirajoshi or Harmonic (JI), the 4:5:6:7 series in just intonation.

**Rate** The step length, from 1/64 up to 1/1, with triplet and dotted values in between.

**Gate** Note length as a fraction of the step.

**Rest** How much of the strand is thinned into rests. Rests fall by Seed, and turning Rest up only ever adds rests to the same line.

**Mute** The steps silenced by hand on the strip, as a bit per step. A muted step never sounds whatever Rest or Mutate do.

**Octaves** With 2 or 3, some steps jump an octave up for range. 1 keeps every base in place.

**Mutate** The chance per bar that one base flips. Zero is a fixed riff; high is restless.

**Velocity** The velocity of every note.

**Seed** Which strand grows. The same Seed always gives the same start.

**Base A** The interval base A plays over Root, 0 to 12 semitones. -1 follows the Chord voicing.

**Base C** The same for base C.

**Base G** The same for base G.

**Base T** The same for base T.

**SwingFollow** Follow makes the pattern take the patch groove from the transport instead of the Swing knob.

**Swing** Delays every second step towards a triplet feel. Used only while SwingFollow is off.

**Chance_1 to Chance_16** How often each step plays, from 0 to 100%.

**Bank** Which of the eight strands plays, A to H. Each bank keeps its own Seed, Mute, base intervals and step chances; Root, Chord, Rate and the knobs are shared. A bank you have not used yet starts as a copy of the one shown.

## Recipe

**Evolving acid** Cord the MIDI outlet into an Acid. Root 45, Chord Phrygian, Rate 1/16, Gate 0.5, Rest 0.3, Octaves 2, Mutate 0.2. Press play and let it run for a few bars; when a strand you like appears, drop Mutate to zero to keep it.

## Related Organisms

Mineral, Substrate, Rhizome
