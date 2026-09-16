# pH

An FM synthesizer with four engines: a six-operator engine that reads DX7 .syx banks, and emulations of the OPN2, OPM and OPL3 chips.

The bank decides the engine. A .syx holds six-operator voices; a .wopn, .tfi or .dmp holds OPN2 instruments for the Nuked-OPN2 core; a .opm holds OPM voices for Nuked-OPM; a .wopl holds OPL3 instruments for Nuked-OPL3. Load one and the right engine plays it: eight voices on the six-operator engine, six channels on the OPN2, eight on the OPM and eighteen on the OPL3. Pitch comes from the patch Tuning, so a microtonal scale plays on chip voices. A patch names its bank rather than carrying it, so share the bank alongside the patch. Cord a PianoRoll, DNA or MidiIn in.

## Parameters

**Patch** Which voice of the bank plays, by name. Choosing a bank starts again at its first voice.

**Bright** How hard the modulators drive their carriers. Centre is the voice as written; up adds harmonics, down closes the tone toward a sine.

**Attack** How long every operator takes to open, around what the voice asks for.

**Release** How long the voice takes to fade after the key lifts.

**Detune** Spread between operators, in cents. Small amounts thicken a voice; large ones beat against themselves.

**Vibrato** Depth of the voice's own low-frequency pitch wobble.

**Speed** How fast that wobble runs.

**Transpose** Shifts the note in semitones. It moves the sounding frequency rather than the note number, so it holds in any tuning.

**Fine** Shifts the note in cents, the same way.

**Level** Output level.

**BendRange** How far the pitch wheel reaches at full travel, in semitones. Two is the common default; zero ignores the wheel.

**Algorithm** Which operator drives which, and which reach the output. The six-operator engine offers 32; the chips offer 8.

**Feedback** How hard the first operator folds back into itself, 0 to 7.

**Op1_Level** Output level of operator 1, and so on for operators 2 to 6. Picking a patch loads its voice into these rows, so what is on screen is what plays.

**Op1_Ratio** Frequency ratio of operator 1 against the note, and so on for 2 to 6.

**Op1_Detune** Fine offset of operator 1, centred at 7, and so on for 2 to 6.

**Op1_Attack** Envelope attack rate of operator 1, and so on for 2 to 6.

**Op1_Decay** Envelope decay rate of operator 1, and so on for 2 to 6.

**Op1_Sustain** Envelope sustain level of operator 1, and so on for 2 to 6.

**Op1_Release** Envelope release rate of operator 1, and so on for 2 to 6.

**Op1_On** Takes operator 1 out of the voice or puts it back, keeping its level meanwhile, and so on for 2 to 6. The chips have four operators, so Op5_On and Op6_On are unavailable while a chip bank is loaded.

**File** The bank to play from, and with it the engine. Empty is the factory bank; a bank dropped in the banks folder joins the list and shadows a shipped bank of the same name.

## Recipe

**Chip electric piano** Load the factory chip bank and pick an electric piano patch. Bright 0.6, Detune 6, Release 0.6, Vibrato 0.1, Speed 0.4. Cord a PianoRoll in and a Fern after it with Feedback around 0.4 for the echo; switch Op4_On off and on while it plays to hear what the top operator adds.

## Related Organisms

Rhizome, Wave, PianoRoll, DNA
