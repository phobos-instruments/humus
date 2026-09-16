# Kick

A synthesized club kick with a pitch envelope, a body decay, a noise click and a drive stage.

A sine at Tune starts higher by Punch and falls over PitchDecay, its body fades over Decay, a short noise burst adds the transient and a saturation stage rounds it off, so the kick is designed rather than sampled. It fires three ways that combine: the Hit button, any note-on at the MIDI inlet from a PianoRoll or a Steps, and 4/4, which fires on every transport beat while playing. Cord the outlet into a Compressor or a Console, and a Follower on it onto a Gain for a sidechain duck.

## Parameters

**Tune** The body's resting frequency, 30 to 90 Hz.

**Punch** How far the pitch starts above Tune. More is a harder attack.

**PitchDecay** P.Dec: how fast the pitch falls to Tune, in milliseconds. Short is a tick, long is a laser.

**Decay** Body decay in milliseconds, the length of the boom.

**Click** Level of the noise transient, for cutting through a dense mix.

**Wood** A short knock of two inharmonic partials that track Tune, gone in about 35 ms. A little adds a mallet character, a lot heads towards a woodblock. 0 leaves the plain sound.

**Drive** Saturation. Push it for a distorted rumble.

**Level** Output level.

**FourFloor** 4/4: fires the kick on every beat while the transport plays.

**Trigger** Hit: fires the kick once. Map it to a pad.

## Recipe

**Four on the floor** Tune 48, Punch 4, PitchDecay 40, Decay 420, Click 0.35, Drive 0.3, 4/4 on. Press play and the kick follows the transport; cord a Follower onto its outlet and the Follower's env onto a Gain on the bass for a duck.

## Related Organisms

SideKick, Microdot, Steps
