# Button

A push button as a patchable organism, with a control outlet and a MIDI note.

Press it and the outlet travels from Off to On, so any knob in the patch can follow it through a control cord or a Control with route, and a note leaves the MIDI outlet. Let go and both return. Latch turns the push into a toggle, which makes it a cheap mute, a scene switch, or a way to hold a Gate open without keeping a finger on it. Map a pad, a key or a footswitch to Press with MIDI learn and the button lives on your controller; cord a Follower's gate onto the Press socket and the patch presses it for you. Anything cabled into its inlet is added to the outlet.

## Parameters

**Press** The button itself. Momentary unless Latch is on.

**Latch** Each press flips the state instead of following the finger.

**Off** The outlet's level while the button is off. It may be larger than On, or negative for a bipolar switch.

**On** The outlet's level while the button is on.

**Slew** How long the outlet takes to reach the new level, in milliseconds. Zero snaps, which clicks through a gain but is what a trigger wants.

**Note** The MIDI note sent while the button is on: note on when it lights, note off when it goes dark.

**Velocity** The velocity of that note.

## Recipe

**Kill switch** Latch on, Off 0, On 1, Slew 40. Cord the outlet onto the Mute of a Gain placed on a drum bus and map Press to a pad with MIDI learn. One tap cuts the drums, the next brings them back, without a click.

## Related Organisms

Slider, Number, Gate, Kick
