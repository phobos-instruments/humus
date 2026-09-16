# Grit

A chip synth built on the Ricoh 2A03 console sound chip, with a cartridge slot for the Famicom expansion chips.

The console alone is two pulse voices with the four hardware duty cycles, a triangle that sits under the lowest held note, a noise channel on the top keys and a 1-bit delta sampler on the bottom keys. Slot in a cartridge and its voices join the pool and take notes first, with the console pulses joining when they are all busy; on the VRC6 the sawtooth goes first. The envelope counts in frames per volume step over sixteen levels, so a fast attack snaps and a slow one climbs audibly. The emulation is NSFPlay's, driven register by register. Cord a PianoRoll, a Riff or a DNA into the MIDI inlet.

## Parameters

**Cartridge** The chip in the slot. 2A03 is the bare console; VRC6 adds two pulses and a sawtooth, MMC5 two pulses, FDS one wavetable voice, N163 four wavetable voices, 5B three fixed squares and VRC7 six two-operator FM voices. Changing it silences held notes.

**Region** NTSC or PAL console. Clock, pitch tables and envelope tick follow, so PAL runs slightly slower and darker.

**Duty** Pulse width for the console, VRC6 and MMC5 pulses: 12%, 25%, 50% or 75%.

**Attack** Frames per volume step on the way up. 0 starts a note at full volume.

**Decay** Frames per volume step on the way down to Sustain.

**Sustain** The level a held note settles at, 0 to 15.

**Release** Frames per volume step after the key is lifted.

**Triangle** Tri layer: puts the console's triangle channel under the lowest held note at the same pitch. It has no volume control, only on or off.

**NoiseKeys** Noise keys: C7 and up play the noise channel instead of a tone, stepping through the sixteen hardware periods as you go up.

**Buzz** Switches the noise into its short-loop mode, a metallic rattle instead of a wash.

**Wave** The wavetable the FDS and N163 play: SIN, TRI, SAW, SQR or ORG.

**Patch** One of the VRC7's fifteen built-in FM instruments, by number.

**Sample** A sound for the delta channel, crushed to 1-bit. Keys below C2 fire it at the sixteen hardware playback rates, pitched down from B1.

**DpcmLoop** Loop: holds the sample round while its key is held.

**Level** Output level.

**BendRange** How far the pitch wheel reaches at full travel, in semitones. Two is the common default; zero ignores the wheel.

## Recipe

**Cartridge lead** Cartridge VRC6, Duty 25%, Attack 0, Decay 6, Sustain 10, Release 3, Tri layer on. Cord a Riff into the MIDI inlet for the line and put a Kick with 4/4 on underneath. Drop a short drum hit into Sample and play it from B1 on a PianoRoll.

## Related Organisms

pH, Silt, Riff
