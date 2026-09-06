# Button

A push button as a patchable object. Press it and the outlet jumps from Off to On (summed onto anything at the inlet), the control value "value" goes to 1 so any parameter in the patch can follow it through a control route, and a note leaves the MIDI outlet. Let go and everything returns. Map a pad, a key or a footswitch to Press via MIDI learn and the button lives on your controller.

Latch turns the push into a toggle: one press switches on, the next switches off, and the note holds for as long as the button is lit. That makes it a cheap mute, a scene switch, or a way to hold a Gate open without keeping a finger on it.

## Parameters

**Press** the button itself. Momentary unless Latch is on.

**Latch** each press flips the state instead of following the finger.

**Off / On** the two levels the outlet travels between. Either may be the larger; a negative Off and positive On give a bipolar switch.

**Slew** how long the outlet takes to reach the new level, in milliseconds. Zero snaps, which clicks through a gain but is what a trigger wants.

**Note** the MIDI note sent while the button is on; note on when it lights, note off when it goes dark.

**Velocity** the velocity of that note.

## Related Organisms

Slider, Number, Gate, Kick
