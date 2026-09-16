# Phono

A phono pre-amplifier: RIAA de-emphasis, cartridge gain and a rumble filter for a turntable plugged into a line input.

Records are cut with the bass reduced and the treble boosted, and a cartridge puts out millivolts, so a record played straight into a line input sounds thin and very quiet. Phono undoes the curve and restores the level. The de-emphasis filter is fitted to the analog RIAA response at the running sample rate, top octave included. Cord it directly after a SoundIn and before a Deck, a Console or a Mixer. If the interface channel has a phono/line switch, set it to line, or the correction is applied twice. Signal flow is de-emphasis, then rumble filter, then gain.

## Parameters

**Curve** RIAA is the standard playback curve. RIAA + IEC adds a gentle 20 Hz roll-off that some phono stages include. Flat skips the equalisation and keeps only the gain, for transfers you will equalise later or for records that predate the standard.

**Gain** Cartridge gain in dB. A moving-magnet cartridge, which is every standard DJ cartridge, wants about 40; a low-output moving-coil cartridge wants about 60. Too high clips.

**Rumble** An 18 Hz high-pass that removes warp wobble and turntable rumble before they eat headroom. On by default; turn it off for measurement work.

## Recipe

**Turntable in** Cord SoundIn into Phono and Phono into a Deck or a Mixer. Curve RIAA, Rumble on, Gain 40 for a moving-magnet cartridge; raise Gain in steps of 5 until a loud passage reads just under full on the meters.

## Related Organisms

Console, Gain, Deck, SoundIn
