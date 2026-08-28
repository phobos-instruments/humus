# Console

An analog summing desk. Where a Mixer adds its inputs together, a Console models the physics that makes a real desk glue: every channel loads a shared power rail, the summed signal leaks across the wiring, and the output transformer softens whatever is driven through it. Nothing here is a per-channel effect - every coupling exists because the channels meet at one bus, and each reacts to what you feed it, moment to moment.

## The channels

Each channel is a stereo pair numbered by its inlets: 1-2 is channel 1, 3-4 channel 2, and so on. A mono source can feed both inlets of a pair to sit centred, or one to sit hard on that side; an unconnected channel contributes silence. The Inputs dropdown sets how many channels the desk sums and re-classes the organism in place, so its name, settings, cords and automation survive. Growing adds channels at the bottom, shrinking drops the highest ones and their cords.

## Signal flow

Gain scales each channel into the stereo bus. Crosstalk bleeds the bus across itself, in the loom, before the bus amp sees anything. Sag pulls the whole bus down according to how hard it is working. Drive saturates the output transformer. Output trims the result.

Because Sag reads the bus after crosstalk but before the iron, loud material on any one channel ducks every other channel - that is the glue. Drive does it too, and it is easy to miss: the iron is one transformer for the whole bus, so whatever is loudest bends the curve every other channel is riding. Put a kick on one channel and a sustained bass on another and the kick's shape turns up in the bass, no crosstalk required. Three controls make that bleed, not one.

## Parameters

**Inputs** how many stereo channels the desk sums: 2 to 8.

**Gain** per-channel level into the bus. The balance control, and what you feed the desk decides how hard the rail works.

**Pan** per-channel balance under each Gain. Centre passes both sides at full level; moving off centre only turns the far side down. The left inlet of each strip doubles as a mono jack - one cord into it feeds both sides, so a mono source sits centred and Pan places it. Cabling only the right side stays right.

**M / S** per-channel Mute and Solo, under each Gain. Soloing silences the others and a channel's own Mute always wins. Cutting channels relaxes the rail, so the survivors sit up.

**Crosstalk** how much of each side's high end bleeds into the opposite side. Subtle by design: it tops out around -30 dB. On centred material it reads as a sheen; it comes alive on material that genuinely differs left to right.

**Sag** how far the summed demand pulls the rail voltage down, springing back as the music breathes. At 0 the rail is stiff. Low and slow it is glue; higher it pumps with the loudest thing on the desk.

**Drive** how hard the transformer is pushed. Its saturation is asymmetric, adding even and odd harmonics, thickening the low end and rounding transients; a DC blocker follows so no offset builds up. At 0 the stage is bypassed, not merely quiet. It ships at 0.3, so the iron is working the moment you patch it in.

**Flavor** voices all three couplings at once.

**Output** master trim, applied last, after all colouring.

**Direct** switches off every coloured stage but keeps the summing and Output - the honest A/B. With it pressed, or all three couplings at zero, the bus is arithmetic again: bit for bit the sum of its channels. To switch the desk out of the patch entirely, use Bypass on the organism bar.

## Flavor

**Clean** transparent summing, barely-there physics. For glue alone.

**British** warm and 2nd-harmonic-forward, with a slow, gluey rail. The default, and the most mixed-sounding.

**American** punchy, with a faster rail and a harder edge. Keeps transients alive under heavy Sag.

**Modern** ultra-linear: minimal crosstalk, a stiff rail, almost no iron.

## Usage

Audition Flavor first, since it re-voices all three couplings and moves the target. Then set the per-channel Gains for balance with Drive and Sag low: the couplings all react to level, so changing a Gain afterwards changes how the desk behaves. Raise Sag until the mix breathes with the kick, and add Drive last for weight. If it pumps too obviously, lower the loudest channel rather than Sag - the rail is only reacting to what you fed it - and trim the level the desk loses at Output rather than at the channels.

## Notes

Console is a bus, not a channel strip: no EQ, no compressor, by design. Put a TransientShaper or SideChain on each source before the desk, as you would patch outboard into a real console's inserts. Sag and Drive are program-dependent, so audition with the mix playing rather than on a single hit.

## Technical discussion

Emulations in track-based hosts must smuggle a rail voltage and a crosstalk bus between isolated plugin instances through global shared memory, and staying race-free costs a block of latency. Humus sums multichannel for real, so every channel is present at the same node on the same sample and the interaction is modelled where it physically happens: no hidden global state, no cross-instance synchronisation, no added latency. Inside, a one-pole high-pass feeds the crosstalk coupling, one asymmetric envelope of the bus demand drives the rail, and the iron is a DC-blocked asymmetric saturator.

## Related Organisms

Mixer, Bus, Gain, SideChain, StereoTool, TransientShaper
