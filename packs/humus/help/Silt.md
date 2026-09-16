# Silt

A three-voice chip synth emulating the MOS SID, the home computer sound chip, through the reSID core.

Both the original 6581 and the cleaner 8580 revision are emulated cycle by cycle, analog filter included. Three voices share one filter as the hardware did; a fourth note steals the oldest voice. Every voice plays the same wave, pulse width and the chip's own four-stage envelope in its native sixteen steps. Pitch comes from the patch tuning to the resolution of the chip's frequency registers, so a microtonal scale plays. Twin mode adds a second chip, as the modded boards did, doubling every note with the pair detuned across the stereo image. Play it over a MIDI cord from a PianoRoll, DNA or MidiIn, or with the keyboard while its editor is focused.

## Parameters

**Model** Which chip: 6581 with the warm, wayward filter, or 8580 with the corrected one. Switching fades the level in briefly to soften the step between the two chips' resting voltages.

**Wave** TRI, SAW, PLS or NSE, the chip's four waveforms.

**PulseWidth** The pulse wave's duty cycle. Only audible on PLS.

**Attack** Envelope attack in the chip's sixteen steps. 0 is instant.

**Decay** Envelope decay to the sustain level, in sixteen steps.

**Sustain** The level held while the note is down, in sixteen steps.

**Release** Envelope release after note-off, in sixteen steps. 15 rings for seconds.

**Cutoff** Where the filter closes, across the register range of the hardware.

**Resonance** How hard the filter sings at the cutoff, in sixteen steps.

**FilterMode** LOW, BAND or HIGH pass.

**Filter** Takes the filter out entirely for the raw voices, brighter and louder.

**Ring** Each voice ring-modulates with its neighbour, metallic and clangorous. It speaks through the TRI wave.

**Sync** Each voice hard-syncs to its neighbour.

**Chips** ONE chip, or TWIN: every note on both chips, one leading each side with a little of the other blended in.

**TwinDetune** How far apart the twin pair is detuned, in cents, straddling the note. Shown as Spread.

**Chain** Wires the first chip's output through the second chip's filter, so the left side arrives dry and the right side through a second analog filter. TWIN only.

**ModelB** Which chip the second one is, B 6581 or B 8580. Mixing a warm original with the clean revision is half the point of two.

**Level** Output level.

**BendRange** How far the pitch wheel reaches at full travel, in semitones. Two is the common default; zero ignores the wheel.

## Recipe

**Chip bass** Model 6581, Wave PLS, PulseWidth 0.25, Attack 0, Decay 9, Sustain 6, Release 5, FilterMode LOW, Cutoff 0.45, Resonance 8. Cord a Riff to the MIDI inlet and add an LFO onto PulseWidth at a quarter of a bar for the classic pulse sweep. Switch to TWIN with TwinDetune 8 to widen it.

## Related Organisms

pH, Acid, Rhizome, PianoRoll
