# MidiFilter

Lets chosen notes and controllers through, holds others back, or turns one into another.

Cord a keyboard, a MidiIn or a sequencer into the inlet and the outlet into an instrument or a MidiOut. It starts with one empty rule marked with a plus: pick Note or CC in it to make a rule, and the next empty one appears below, up to eight. Pick Off to take a rule away. Once there are more rules than the panel shows, it scrolls. Click a note box and play a key on your MIDI keyboard to pick that note. Each rule listens for a note or a controller, or a range of them, and decides what happens: Block stops it, To note sends it on as another note, To CC sends it on as a controller. Rules are read top to bottom and the first one that matches wins, so a narrow rule above a wide one makes an exception. A range moves together: C1 to B1 sent to C4 plays the whole octave three octaves up. A released note always reaches the note its press went to, even if you change a row while it is held.

## Parameters

**In1** What row 1 listens for: Off, Note or CC. And so on for rows 2 to 8.

**Low1** The lowest note or controller number row 1 matches. And so on for rows 2 to 8.

**High1** The highest note or controller number row 1 matches. Set it equal to Low for a single one. And so on for rows 2 to 8.

**Do1** What row 1 does with a match. Block stops it. To note sends a note, and a controller becomes a note that plays when its value reaches half way and releases when it falls back below. To CC sends a controller, and a note becomes that controller at its velocity when pressed and at zero when released. And so on for rows 2 to 8.

**Target1** The note or controller row 1 sends its lowest match to; the rest of the range follows in order. And so on for rows 2 to 8.

**Others** What happens to everything no row matches, including pitch bend, pressure and program changes. Pass lets it through. Block keeps only what the rows send.

## Recipe

**A pad that sweeps a filter** Row 1: In Note, Low and High C1, Do To CC, Target 74. Cord the pad controller in and the outlet into a synth with its cutoff mapped to CC 74: hitting C1 harder opens the filter further, and letting go closes it.

**Only the drums** Row 1: In Note, Low C1, High B2, Do To note, Target C1. Others Block. Everything above the two bottom octaves, and every knob, stays out of the drum machine.

## Related Organisms

MidiIn, MidiOut, MidiMonitor, Cluster
