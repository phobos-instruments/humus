# pH

How acid or alkaline the soil is. pH is an FM synthesizer with two real engines: a six-operator engine that speaks the classic DX7 .syx bank format, and the OPN2, the Mega Drive's YM2612 sound chip, emulated by the Nuked-OPN2 core.

The bank decides which one you hear A bank already knows what it is: a .syx holds six-operator voices, and a .wopn, .tfi or .dmp holds chip instruments. Load one and the right engine plays it. Bank picks the sounds, Patch picks one by name, and a preset remembers both along with everything else on the panel.

The six-operator engine is the core the Dexed community plays: eight voices, and any 32-voice .syx bank ever made for that format. The chip is the console's own, six channels of polyphony with the ladder-effect converter of the original hardware.

Pitch comes from the patch tuning, chip included, so a microtonal scale plays on hardware voices that never supported one.

Play it over a MIDI cord from a PianoRoll, DNA or MidiIn, or with the QWERTY keyboard while its editor is focused.

## Adding banks

Two factory banks are built in, plus two chip banks of roughly 185 and 700 named instruments. Drop your own into the banks folder and they join the list; one you install shadows a shipped bank of the same name. A patch names its bank rather than carrying it, so share the bank alongside the patch. Thousands of .syx voice banks and chip instruments circulate freely online, and all of them load. Clearing Bank returns to the factory voices.

## The dice and Evolve

The dice hands you another sound outright: another bank, another voice inside it, and fresh shaping. Evolve wanders the sound already in front of you and deliberately stays in its bank and patch - the two gestures answer different questions.

A knob you have settled on can sit the roll out: right-click it and choose Exclude from Random. It wears a small padlock afterwards, and neither the dice on this instrument nor the one on the toolbar will touch it again until you lift it. The setting is saved with the patch.

## Editing the voice

The operator rows are the sound itself. Each operator has a level, a frequency ratio, a detune and a four-stage envelope, and the routing that joins them is set by Algorithm and Feedback. Picking a patch loads that voice into the rows, so what is on screen is what is playing; move a fader and the sound follows. Picking another patch loads that one in its place.

Each operator's number is also its switch: turn one off to take it out of the voice, on to put it back. Its level is kept meanwhile, so a mute is not the same as turning a level down.

The chip has four operators, so the fifth and sixth switches are unavailable while a chip bank is loaded, and it offers eight algorithms rather than the six-operator engine's thirty-two.

## Parameters

**Bank** which bank of sounds to play from, and with it which engine. Empty is the built-in factory bank.

**Patch** which voice of that bank to play, by name. Choosing a bank starts again at its first voice, since the same number means a different sound in every bank.

**Bright** how hard the modulators drive their carriers. Centre is the voice as its author wrote it; up adds harmonics, down closes the tone toward a sine.

**Attack** how long every operator takes to open, around what the voice asks for.

**Release** how long it takes to die away after the key is let go.

**Detune** spread between operators, in cents. Small amounts thicken a voice; large ones beat against themselves.

**Vibrato** depth of the voice's own low-frequency oscillator, as pitch.

**Speed** how fast that oscillator runs.

**Transpose** shifts the note in semitones, and Fine in cents. Both move the sounding frequency rather than the note number, so they hold in any tuning.

**Level** output level.

**Algorithm** which operator drives which, and which reach the output.

**Feedback** how hard the first operator folds back into itself.

**Op1..Op6** per-operator level, ratio, detune and envelope - the voice.

**Bank** a voice bank or chip instrument to load. Empty means factory.

## Related Organisms

Rhizome, Wave, PianoRoll, DNA, MidiIn
