# Graft

A sample stitcher that cuts up to eight sound files into fragments and chains one fragment per slot into a new sample.

Load sound files into the slots, choose how they are cut and how the slots pick their fragments, and the stitched result is the instrument: it plays from the MIDI inlet with C4 at its own speed, or drones on its own with Play. Every cut moves to the nearest zero crossing and neighbouring fragments overlap on an equal-power crossfade, so a join between unrelated recordings passes without a click. The same Seed always rebuilds the same result, so a saved patch reopens as it was, and the dice rolls a new one without touching the other settings. Cord a PianoRoll or a DNA into the MIDI inlet, or set Play on and cord the outlet into a Fern or a Filter.

## The map

Each band is coloured by the file it came from, matching the slot above. Click a band to hear that fragment alone. Drag up or down to retune it, drag sideways to step through the same file's other fragments, and hold Shift while dragging sideways to step through the files instead. Right-click to reverse or mute the slot, roll it again on its own, or keep it. A kept slot holds its fragment while everything around it re-rolls.

## Parameters

**File1** A source file to cut up, and so on for File2 to File8. Any sound file.

**Slices** How many slots the result has, 4 to 64.

**Order** How the slots choose. Random takes any fragment of any file; In order and Round robin walk the loaded files slot by slot; Blocks gives each file a contiguous run; Mirror builds the first half at random and folds it back reversed; One source rearranges only the file Only points at.

**Cut** Where fragments come from. Free gives every file as many pieces as there are slots, Grid cuts at a note value of the transport tempo, Onsets cuts at transients.

**Div** The fragment length in Grid mode, as a note value.

**Length** In Free mode, how much of each natural fragment is taken, 10 to 200 percent. Short stutters, long overlaps.

**Sense** In Onsets mode, how many transients count as cuts.

**Xfade** The overlap at every seam, in milliseconds.

**Seed** Shown as Creature. The same number always rebuilds the same result.

**Bars** 0 leaves the result whatever length it comes out. Any other number resamples it to land on that many bars at the current tempo, which shifts pitch by up to an octave; the status line says by how much.

**Normalize** Lifts the finished result to just under full scale.

**Source** Shown as Only: which file One source uses.

**Play** Drones the result continuously without a note. Edits rebuild underneath it without a break.

**Attack** Envelope attack in milliseconds for every note.

**Decay** Envelope decay in milliseconds.

**Sustain** Envelope sustain level.

**Release** Envelope release in milliseconds.

**Loop** A held note runs round instead of stopping at the end.

**Reverse** Plays from the end.

**Speed** Playback rate, a quarter to four times, without moving the keyboard.

**Start** Where in the result a note begins. Drag the strip under the map to place it.

**Level** Output level.

**BendRange** How far the pitch wheel reaches at full travel, in semitones. Two is the common default; zero ignores the wheel.

**Voices** How many notes can sound at once, up to 16.

**SliceEdits** The per-slot retune, reverse and mute edits made on the map, stored with the patch.

**Pins** The kept slots, stored with the patch.

## Recipe

**Stitched break** Load a drum loop, a vocal and a field recording into three slots. Cut Onsets, Sense 0.5, Slices 16, Order Random, Bars 2, Play on. Roll the dice until one slot lands well, right-click it and keep it, then roll again until the rest fits. Cord the outlet through a Fern.

## Related Organisms

Leafcutter, Sampler, Paulstretch, PianoRoll
