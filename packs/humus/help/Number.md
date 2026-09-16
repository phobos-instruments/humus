# Number

A number box that shows one value live, takes it from a control inlet and sends it on from a control outlet.

Cords carry sound and numbers land on knobs; a Number is where you watch a number on its way to a knob. Cord a Follower, a Slider, an RNG or another Number onto its inlet and the box shows what arrives, unscaled; type a value and it holds until something new comes in. Any knob in the patch can follow the outlet: draw a control cord onto a socket, or right-click the knob and pick Control with. Because Value is an ordinary parameter, automation lanes and MIDI control can drive it too, which makes Number the bridge between those and every socket in the patch.

## Parameters

**Value** The number the box holds. The inlet sets it, typing sets it, and the outlet sends it on.

**Format** The shape the number leaves in. Any sends exactly what it holds, Whole rounds to an integer, and 8-bit, 8-bit signed, 16-bit and 16-bit signed round and clamp it to that width, so the box shows what actually leaves.

## Recipe

**Watched sidechain** Cord a Follower's outlet onto a Number's inlet and the Number's outlet onto the Gain socket of a Gain on the bass. The box reads the envelope as it moves, so you can see the level a duck reaches before you set the route's range.

## Related Organisms

Slider, Sig, Follower, Gain
