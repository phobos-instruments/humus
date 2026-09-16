# Trellis

A monophonic pitch corrector that pulls a voice or lead onto the notes of a key and scale.

It tracks the incoming fundamental, snaps it to the nearest allowed note of the chosen Key and Scale, and shifts the audio there with a crossfading delay-line shifter, one voice at a time. Speed sets how fast the glide to the target is: slow keeps the natural drift, fast is the hard-tune effect. The note grid comes from the patch tuning, so a Tuning organism set to 19-EDO puts the scale masks over 19 degrees rather than 12. The wet path carries a short delay. Cord a mono vocal from a SoundIn, or a Decomposer, in.

## Parameters

**Strength** How far toward the target the pitch is pulled. 0 leaves the performance alone; 1 snaps fully onto the grid.

**Speed** How fast the pitch glides to the target. Low is transparent correction; near maximum is the robotic snap.

**Key** The tonic, C to B.

**Scale** The notes the pitch may land on: Chromatic, Major, Minor, the modes, Harmonic and Melodic Minor, both pentatonics, Blues, Whole Tone and Hirajoshi.

**Mix** How much corrected signal is heard against the dry.

## Recipe

**Natural tune** Key to the song's key, Scale Major or Minor, Strength 0.7, Speed 0.4, Mix 1. For the effect instead: Scale Chromatic, Strength 1, Speed 1. Cord the output to a Verbatim or a Fern so the correction sits in a space.

## Related Organisms

Decomposer, Tuning
