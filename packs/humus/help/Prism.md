# Prism

A prism for sound. The input is split into its harmonics the way glass splits light into colours, and each colour gets a fader: unity passes the harmonic exactly as heard, zero subtracts it, and pushing above unity grows it - including harmonics the source never had. An added harmonic rides the fundamental's own envelope, so it blooms and dies with the note instead of droning over it. Feed it a voice, a bass line, or a Wave.

## Parameters

**Mode** Refract rebuilds the sound one partial at a time (true additive resynthesis over a pitch tracker) - this is the engine that can add harmonics and disperse them, and it wants pitched, one-note-at-a-time material. Filter is a harmonic EQ on the spectrum itself: it handles chords, drums and noise, stereo intact, but can only reshape the harmonics that are already there.

**H1..H16** the fader per harmonic, 0 to 2 with unity in the middle.

**Disperse** Refract only: stretches the spacing between partials, so harmonic k moves beyond k times the fundamental. Positive values slide the sound toward bells and metal; a touch of negative pulls the stack into a tighter cluster. No EQ can do this - the partials are being re-struck, not filtered.

**Residual** Filter only: the gain of everything between the harmonics. At 0 a sound is purified down to its tone; with the faders at 0 and Residual up, the tone is removed and the breath, bow noise and room remain.

**Mix** dry/wet. The dry path is delayed to match, so any blend stays phase-coherent.

**Level** output level of the refracted signal.

## Signal flow

Input -> pitch tracker -> split (heterodyne bank or STFT) -> faders -> recombine -> Mix with the delayed dry -> Output. The editor's readout shows the note the prism is locked onto, named in the patch's Tuning.

## Notes

Both modes carry the same small delay, and the patch compensates for it. Refract's output is the oscillator bank alone, in mono: when the tracker hears nothing pitched, the bank fades to silence rather than chasing noise. Switch to Filter for material without one clear pitch.

## Related Organisms

Wave, Decomposer, Tuning
