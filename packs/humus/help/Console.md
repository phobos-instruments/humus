# Console

An analog summing desk: a mixer whose channels share one power rail and one output transformer, so they react to each other.

Where a Mixer adds its inputs, a Console models what makes a desk glue. Each channel is a stereo pair numbered by its inlets, 1-2, 3-4 and so on. Gain and Pan place each channel on the bus, Crosstalk leaks the bus across itself, Sag pulls the whole bus down according to how hard it is working, and Drive saturates the transformer. Because Sag and Drive read the summed bus, a loud kick on one channel ducks and bends every other channel; that is the glue, and it is why the desk should be auditioned with the mix playing rather than on a single hit. It is a bus, not a channel strip: there is no EQ or compressor, so put a TransientShaper or a SideChain on each source before it. Cord it where a Mixer would go, before the SoundOut.

## The desk

The Inputs dropdown sets how many stereo channels the desk sums, 2 to 8, and re-classes the organism in place so its name, settings, cords and automation survive. Growing adds channels at the bottom; shrinking drops the highest ones and their cords. One cord into a channel's left inlet feeds both sides, so a mono source sits centred and Pan places it.

## Parameters

**Output** Master trim, applied last, after all colouring.

**Direct** Switches off every coloured stage but keeps the summing and Output. With it on the bus is the plain sum of its channels, for an honest A/B.

**Drive** How hard the transformer is pushed. Its saturation is asymmetric, thickening the low end and rounding transients; at zero the stage is out of the path.

**Crosstalk** How much of each side's high end bleeds into the opposite side. Subtle by design, topping out around -30 dB; it shows most on material that differs left to right.

**Sag** How far the summed demand pulls the rail down, springing back as the music breathes. Low and slow is glue; higher it pumps with the loudest thing on the desk.

**Flavor** Voices all three couplings at once. Clean is transparent summing. British is warm and gluey with a slow rail. American is punchy with a faster rail and a harder edge. Modern is near-linear: a stiff rail, little crosstalk, almost no iron.

**Gain_1-2** Level of channel 1 into the bus. And so on for 3-4 to 11-12.

**Pan_1-2** Balance of channel 1. Centre passes both sides at full level; moving off centre only turns the far side down. And so on for 3-4 to 11-12.

**Mute_1-2** Silences channel 1, which also relaxes the rail for the others. And so on for 3-4 to 11-12.

**Solo_1-2** Silences every channel but this one. A channel's own Mute always wins. And so on for 3-4 to 11-12.

## Recipe

**Glue a mix** Pick Flavor first, since it moves the target for everything else. Set the channel Gains for balance with Drive and Sag low, raise Sag until the mix breathes with the kick, then add Drive last for weight. If it pumps too obviously, lower the loudest channel rather than Sag, and make up any lost level at Output.

## Related Organisms

Mixer, Bus, SideChain, TransientShaper
