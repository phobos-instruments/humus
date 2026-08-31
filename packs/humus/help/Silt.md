# Silt

The finest sediment in the soil. Silt is the home computer's sound chip: the MOS 6581 - the SID that gave the Commodore 64 its voice - and its cleaner 8580 revision, both emulated by the reSID core, cycle by cycle, analog filter included.

Three voices share one filter, the way the hardware did. Each note takes a voice; a fourth note steals the oldest. Every voice plays the same shape - a wave, a pulse width, and the chip's own four-stage envelope in its native sixteen steps - which is what playing this chip polyphonically always meant.

The filter is the famous part. The 6581's is warm, misbiased and slightly broken in a way three decades of music grew to love; the 8580's is the corrected one. Switch Model and the same patch wears both. Low, band and high modes, a sixteen-step resonance, and a cutoff that sweeps the register range the chip actually had.

Ring modulation and hard sync are the chip's own tricks: each voice bends its neighbour, so with three voices sounding they chew on each other exactly as the register map intended. Ring modulation speaks through the triangle wave; sync works on all of them.

Pitch comes from the patch tuning, so a microtonal scale plays on a chip that never heard of one - to the resolution of its sixteen-bit frequency registers, which is finer than a cent.

Twin mode is the second chip the modders soldered in. The home computer grew MIDI through cartridge interfaces, and the boldest boards put a second SID at a mirrored address. Here the twin doubles every note, the pair detuned to straddle the pitch by Spread, one chip leading each side with a taste of the other blended in - so the beat is there whether you take one cord or two. The second chip has its own model switch, because the boards people actually built mixed a warm original with a clean revision as often as not. Chain is the other trick from those boards - the first chip's output wired into the second's audio-in pin, so the left side arrives dry and the right side arrives again through the second chip's analog filter.

Play it over a MIDI cord from a PianoRoll, DNA or MidiIn, or with the QWERTY keyboard while its editor is focused.

## Parameters

**Model** which chip: the original with the warm, wayward filter, or the revision with the corrected one. Switching lands with a low thump - the two chips rest at different voltages, and the step between them is softened here rather than hidden, since speaking through DC steps is how this chip played samples in the first place.

**Wave** triangle, sawtooth, pulse or noise - the chip's four.

**Width** the pulse wave's duty cycle. Only audible on pulse.

**Attack, Decay, Sustain, Release** the envelope, in the chip's own sixteen steps each. Attack 0 is instant; release 15 rings for seconds.

**Cutoff** where the filter closes, across the register range of the hardware.

**Reso** how hard the filter sings at the cutoff.

**Filter mode** low, band or high pass.

**Filter** takes the filter out entirely - the raw voices, brighter and louder.

**Ring** each voice ring-modulates with its neighbour. Metallic, clangorous; speaks through the triangle wave.

**Sync** each voice hard-syncs to its neighbour.

**Chips** one chip, or the twin board: every note on both, one chip leading each side.

**B 6581 / B 8580** which chip the second one is. Mixing a warm original with the clean revision is half the reason to own two.

**Spread** how far apart the pair is detuned, in cents, straddling the note.

**Chain** wires the first chip's output through the second chip's filter, the way the twin boards did on the audio-in pin. Twin only.

**Level** output level.

## Related Organisms

pH, Acid, Rhizome, PianoRoll, DNA, MidiIn
