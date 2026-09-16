# Prism

A harmonic resynthesiser that splits the input into sixteen harmonics and gives each one a fader.

Unity passes a harmonic as heard, zero subtracts it, and above unity grows it, including harmonics the source never had; an added harmonic rides the fundamental's own envelope, so it blooms and dies with the note. Refract rebuilds the sound one partial at a time over a pitch tracker and wants pitched, one-note-at-a-time material; Filter is a harmonic equaliser on the spectrum itself and handles chords, drums and noise. The dry path is delayed to match, so any Mix blend stays phase-coherent. The readout shows the note the tracker is locked onto, named in the patch's Tuning. Feed it a voice, a bass line or a Wave.

## Parameters

**Mode** Refract resynthesises the partials in mono and fades to silence when nothing pitched is heard. Filter reshapes the harmonics already present, stereo intact, and cannot add new ones.

**Dispersion** Refract only. Stretches the spacing between partials so a harmonic lands beyond its whole-number multiple of the fundamental; positive slides toward bells and metal, a touch of negative pulls the stack tighter.

**Residual** Filter only. The gain of everything between the harmonics. At 0 a sound is reduced to its tone; with the faders at 0 and Residual up, the tone goes and the breath and room remain.

**Mix** Dry against wet.

**Level** Output level of the wet signal.

**Harm_1** The fader for the first harmonic, 0 to 2 with unity in the middle, and so on for Harm_2 to Harm_16.

## Recipe

**Bell from a bass** Mode Refract, and cord a Rhizome playing single notes in. Harm_1 at 1, Harm_2 to Harm_5 at 1.6, the rest at 0, Dispersion 6, Mix 1. Each note now rings as a bell that follows the bass's own envelope.

## Related Organisms

Wave, Decomposer, Tuning
