# FrequencyShifter

The unmooring. This is not a pitch shifter, and the difference is the whole organism: a pitch shifter multiplies every frequency by the same amount, so the harmonics stay in step and the sound keeps its identity at a new pitch. This adds the same number of hertz to every frequency instead. A harmonic series at 100, 200, 300 shifted by 50 becomes 150, 250, 350 - no longer a harmonic series at all.

The sound stops being a note and becomes a bell, a scrape, a piece of metal.

Small shifts of a few hertz are a slow phasing shimmer; larger ones destroy pitch entirely and leave you the texture. Automate it and material slides between the two.

## Parameters

**ShiftFrequency** how many hertz to add, positive or negative. At zero nothing happens. Negative shifts push the partials together rather than apart, and low harmonics can be driven down through zero and back up, which is where the strangest sounds are.

**WetDryMix** how much of the shifted sound is heard against the original. Small shifts against the dry signal beat slowly against it; full wet abandons the original pitch completely.

## How it works

Shifting every frequency by a fixed amount requires knowing which way each one is rotating, which a plain audio signal does not tell you. The organism builds an analytic version of the signal - the original in one hand, and a copy phase-shifted by a quarter cycle at every frequency in the other - using a Hilbert transform filter. With both, each frequency can be multiplied against a quadrature oscillator so that the unwanted mirror image cancels and only the upward or downward shift survives. That cancellation is why this is called single-sideband, and why it sounds clean rather than like ring modulation.

The oscillator is shared by both channels, so the stereo image is preserved.

## Related Organisms

Prism, SpectralMorph, Chorus
