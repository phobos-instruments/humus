# Crossover

A frequency splitter that sends a signal out in bands, one stereo pair of outlets per band, lowest first.

The splits are phase-matched, so cord every band into one inlet and they sum back to the original sound flat. Everything between the split and the merge happens to one band only: a Compressor on each band is multiband dynamics, a Fern on the mids alone is an echo the bass stays out of, a Follower on the low outlet turns the kick into a control signal untouched by the hats. A new Crossover is three-way; the Bands dropdown on its editor reshapes it in place between 2 and 5, keeping cords and split settings, with a Freq3 and a Freq4 appearing as bands are added. Process the bands and merge them, but avoid mixing the merged result with the dry signal it came from: a crossover rotates phase, so the two cancel oddly when layered.

## Parameters

**Freq1** The lowest crossover frequency, in Hz.

**Freq2** The next crossover frequency up. The splits keep their order, so a split cannot be dragged below the one before it.

**Slope** How steep the seam between bands is. 24 dB/oct is the standard, phase-matched and flat-summing. 12 dB/oct is gentler for tonal blending; alternate bands are polarity-inverted in this mode so it still sums flat, which is inaudible on its own. 48 dB/oct is nearly a wall, so each band can be driven hard on its own.

## Recipe

**Three-band drums** Freq1 120, Freq2 2000, Slope 24 dB/oct. Cord a drum Mixer in, a Compressor on each pair of outlets with a slower attack on the low band, and all three Compressors into the next inlet, which sums them back.

## Related Organisms

ParaEQ, Filter, StereoTool, Follower
