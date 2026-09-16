# Bloom

The shimmer reverb: a modulated reverb tank with a pitch shifter inside its feedback loop.

Every pass around the tank is transposed again, so a tail climbs by an octave, then two, until the damping takes it. The balance between the plain feedback and the shifted feedback is Shimmer: at zero this is an ordinary reverb, at full the tail is all climb and no body. Feedback sets how long the whole thing rings. Turn Shift negative and the tail sinks instead. Cord it after a Rhizome, a Wave or a Sampler, and put a Fern before it for a tail that also echoes.

## Parameters

**Mix** Dry against wet.

**Shift** How far the shifter transposes each pass, in semitones. 12 is the classic octave, 7 stacks fifths, -12 sinks.

**Shimmer** How much of the shifted signal is fed back around the tank against the unshifted tail.

**Feedback** How much of the tank returns to its own input. This is the tail length.

**Size** The room's dimensions, from small to vast.

**Diffusion** How quickly the early echoes blur together. Low keeps discrete echoes audible inside the tail.

**LowCut** Removes low end from the tank input, to keep the swell out of the bass.

**HighCut** Darkens the tail on each pass. Lower is a shorter, warmer tail.

**ModRate** The speed of the movement inside the tank, in Hz.

**ModDepth** How far that movement goes. A little keeps the tail alive, a lot turns it to water.

**Mode** The room type. Bloom is small and close, Hall is natural, Cathedral is vast, Cloud sits between with extra diffusion.

**Voice** The shifter arrangement. Single follows Shift, Dual adds a second voice moving the opposite way, Stacked adds a second voice at twice the shift, and Off is a plain reverb whatever Shift says.

**Color** The tilt of the tail. Bright keeps the shifted copies open, Dark folds them down, Neutral sits between.

## Recipe

**Pad halo** Mode Cathedral, Voice Single, Shift 12, Shimmer 0.6, Feedback 0.6, HighCut 6000, Color Dark, Mix 0.4. Cord a slow Rhizome pad in and hold a chord for a bar or two while the octaves build.

## Related Organisms

Fern, Prism, SpectralFreeze
