# Sampler

A polyphonic zone-mapped sample player with eight slots, sound bank loading and recording off the patch.

Load up to eight sound files, give each a root note and play them over MIDI from a MidiIn, a PianoRoll or the on-screen keyboard. Or load a whole sound bank file (.sf2) and play its instruments; a loaded bank is the instrument, and the slots play only when no bank is loaded. Cord a signal into the audio inlet and Record captures a take straight into a slot, as a sound file that saves with the patch. It opens coarse, twelve bits at 32 kHz, in the tradition of the late-eighties rack sampler; the preset rail walks from Ancient to Pristine, moving Bits and Rate and nothing else. Velocity scales level, and past Polyphony the oldest voice is stolen.

## Slots and banks

In Pitched mode the zones split the keyboard at the midpoints between their root notes and notes are pitched by resampling from the nearest root; one zone spans the whole keyboard. In Kit mode each zone plays only on its root key at native rate, for one-shots on individual keys. A bank brings its own key splits, loop points and envelopes, and the envelope knobs then scale those rather than replace them. Takes land in a samples folder under Documents.

## Parameters

**Mode** Pitched spreads the zones across the keyboard by root note. Kit plays each zone on its root key only.

**Gain** Output level.

**BendRange** How far the pitch wheel reaches at full travel, in semitones. Two is the common default; zero ignores the wheel.

**Attack** Rise time of every note in milliseconds. With a bank loaded it scales the zone's own attack, so the default plays the bank as written.

**Decay** Fall time to the Sustain level in milliseconds, scaled the same way with a bank.

**Sustain** The held level, 0 to 1.

**Release** Fade time after the key is released, in milliseconds.

**Polyphony** The number of voices sounding at once. Past it the oldest voice is stolen.

**Loop** Repeats the sample while a key is held. A file with its own loop points uses them; otherwise it loops end to end.

**File1** The sound file in slot 1, and so on for File2 to File8.

**Root1** The MIDI note slot 1 plays at native pitch, 60 by default, and so on for Root2 to Root8. A file that records its own key sets it on load; for a kit, set each root to the key you want it on.

**FileBank** A sound bank file. While one is loaded it is what plays.

**Preset** Which instrument in the bank answers, listed by name.

**RecSlot** The slot a take lands in.

**RecordDuration** Stops a take by itself after that many milliseconds. At 0 a take runs until you stop it or until the thirty-second ceiling.

**Record** Starts a take from the audio inlet. Press it again to stop.

**Bits** Sample word length, 8 to 24. Below 24 the file is stored coarsely, the way early hardware samplers did, and moving it re-reads the file.

**Rate** Sample rate in kHz, 4 to 48. At 48 the file is left alone.

**Monitor** Passes the inlet through to the outputs so you hear what you are about to take. Off by default, so a Sampler already on a mixer does not double the signal.

**Threshold** Holds an armed take until the input reaches that level, drawn as the mark over the meter. At the far left it records the moment you arm.

## Recipe

**Kit from the patch** Mode Kit, Root1 36, Root2 38, Root3 42. Cord a Kick into the audio inlet, RecSlot 1, Threshold 0.05, Record on, and hit the Kick once; repeat for the other slots with other sources. Cord a Riff or a PianoRoll into the MIDI inlet and play the kit.

## Related Organisms

PianoRoll, MidiIn
