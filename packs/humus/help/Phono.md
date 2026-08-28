# Phono

A true phono pre-amplifier. Plug a turntable into your audio interface's line input, put Phono right after SoundIn, and the record plays back at the right level with the right tone - the RIAA de-emphasis curve, cartridge gain and a rumble filter, exactly the job the phono stage in a mixer or amplifier does.

Why it is needed: records are cut with the bass reduced and the treble boosted (the RIAA curve, standard since 1954), and a cartridge puts out millivolts, not line level. Without correction a record sounds thin, screechy and very quiet. Phono undoes the curve and restores the level.

"True" means measured, not flavoured: the de-emphasis filter is fitted to the exact analog RIAA response at your running sample rate, and the test suite holds it to a fraction of a dB across the audio band - including the top octave at 44.1 kHz, where naive digital RIAA filters go badly wrong.

## Parameters

**Curve** RIAA is the standard playback curve. RIAA + IEC adds the IEC amendment's gentle 20 Hz roll-off (a first-order subsonic cut some phono stages include). Flat skips the EQ and keeps only the gain - useful for archival transfers you plan to equalise later, or for 78s that predate the RIAA standard.

**Gain** cartridge gain in dB. A moving-magnet (MM) cartridge - every standard DJ cartridge is one - wants about 40 dB. A low-output moving-coil (MC) audiophile cartridge wants about 60 dB. Too low sounds quiet; too high clips.

**Rumble** an 18 Hz subsonic filter (3rd-order Butterworth) that removes warp wobble and turntable rumble before they eat headroom. On by default; turn it off for measurement work.

## Signal flow

Input -> RIAA de-emphasis -> Rumble filter -> Gain -> Output.

## Notes

The turntable must be plugged into a line input for this to work - if your mixer or interface channel has a phono/line switch, set it to line, or the RIAA correction will be applied twice. Software cannot provide the 47 kOhm cartridge loading a hardware phono input presents; with most MM cartridges the audible difference is a small treble tilt. Everything else - curve, gain, rumble - is fully handled here.

## Related Organisms

Console, Gain
