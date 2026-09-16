# DigiRust

A lo-fi channel that models a cheap converter going bad: fewer bits, a slower clock, a wobbling clock and hiss.

Bits coarsens the signal into fewer levels and Rate holds each sample longer so the top end aliases down into grit. Jitter wobbles the clock so hold lengths vary sample to sample, and Noise adds a hiss that rides the signal and ducks out in silence, so an idle patch stays quiet. Tone tilts the result dark or present, Mix blends it against the dry input for parallel grit, and a DC blocker keeps the output centred. The dice roll Bits, Rate, Jitter, Noise and Tone but leave Mix and Level alone, so a roll changes the colour, never the balance of the patch. Cord it after a Sampler or a Drums organism, or before a Fern for a degraded echo.

## Parameters

**Bits** Word length, 1 to 16, in whole steps. Lower is coarser.

**Rate** Sampling clock in Hz. At the top of its travel it is out of the path; low is crunchy.

**Jitter** Clock wobble. Zero is a steady clock; up is warble and smear.

**Noise** Signal-ducked hiss. Zero is silent.

**Tone** Dark to present tilt around the middle.

**Mix** Dry to rusted blend.

**Level** Output level.

## Recipe

**Old sampler drums** Bits 10, Rate 22000, Jitter 0.1, Noise 0.15, Tone 0.4, Mix 1. Cord a Sampler playing a break in and the top end dulls into the classic early-sampler crunch; pull Rate down to 8000 for the breakdown.

## Related Organisms

Sampler, Fern, Filter
