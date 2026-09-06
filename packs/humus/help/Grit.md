# Grit

The Famicom's voice: a Ricoh 2A03 - or its PAL twin, the 2A07 - with a cartridge slot. The console chip alone is five voices: two pulse waves with the four hardware duty cycles, the bare 4-bit staircase triangle, the LFSR noise channel, and the 1-bit delta sampler. Slot in a cartridge and its expansion audio joins the pool, the way Famicom games wired extra synthesizers through the cart connector. The emulation is NSFPlay's, driven register by register.

## The console

Two pulses take your notes first, oldest note stolen when the chip runs out. **Duty** picks their width - the hardware's own 12.5, 25, 50 and 75 percent - and the ADSR shapes them in the chip's sixteen volume steps, ticked at frame rate the way NES drivers actually articulated notes, so a fast attack snaps and a slow one climbs the staircase audibly.

**Triangle** shadows the lowest held note an octave of body below the pulses. The hardware gave it no volume knob, so neither do we: it is on or off, and that is its charm.

**Noise keys**: C7 and up play the noise channel instead of a tone, stepped down the hardware's sixteen periods as you go up the keys. **Buzz** flips it into the short-loop mode - the metallic 93-step rattle instead of white wash.

**Sample** loads any sound into the delta channel: it is crushed to 1-bit DPCM, exactly the format the console streamed, and keys below C2 fire it at the hardware's sixteen playback rates - your kick on B1, pitched down from there. **Loop** holds it while the key is held.

## The cartridges

- **VRC6** - two more pulses with finer widths and the famous sawtooth: three extra voices.
- **MMC5** - two more straight pulses.
- **FDS** - the disk system's wavetable voice; **Wave** picks its table.
- **N163** - four wavetable voices from the multiplexed Namco chip, same **Wave**.
- **5B** - three flat PSG squares.
- **VRC7** - six two-operator FM voices; **Patch** picks among the chip's fifteen built-in instruments.

One cartridge sits in the slot at a time; changing it silences held notes, like swapping the game.

## Parameters

**Region** is the console itself: NTSC (2A03) or PAL (2A07) - clock, pitch tables and envelope tick all follow, so PAL runs a shade slower and darker, as it did.

**Attack / Decay / Sustain / Release** count in frames per volume step, sixteen levels top to bottom. **Level** is the output volume; the mix uses the console's nonlinear DAC curves, so channels crowd each other exactly as they did on hardware.
