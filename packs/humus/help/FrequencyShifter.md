# FrequencyShifter

Adds a fixed number of hertz to every frequency in the signal.

A pitch shifter multiplies every frequency by the same ratio, so harmonics stay in step and the sound keeps its identity. This organism adds the same number of hertz to each one instead: a series at 100, 200 and 300 shifted by 50 becomes 150, 250 and 350, which is no longer harmonic. Small shifts of a few hertz give a slow phasing shimmer against the dry signal; larger shifts turn a note into a bell or a scrape. It works as a single-sideband shifter, so the result stays clean rather than sounding like ring modulation, and the same oscillator drives both channels so the stereo image holds. Cord it after a Rhizome or a Sampler, or after a Fern so the repeats slide away from the note.

## Parameters

**ShiftFrequency** How many hertz to add, positive or negative. Negative shifts push the partials together, and low harmonics can be driven down through zero and back up.

**WetDryMix** How much of the shifted signal is heard against the original. Small shifts against the dry signal beat slowly; full wet abandons the original pitch entirely.

## Recipe

**Slow shimmer** Cord a sustained pad in. ShiftFrequency 3, WetDryMix 0.5, and the shifted copy beats against the dry pad about three times a second. Cord an LFO onto ShiftFrequency with a small Amplitude to make the beating drift.

## Related Organisms

Prism, SpectralMorph, Chorus
