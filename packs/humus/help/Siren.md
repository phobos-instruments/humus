# Siren

A sound-system siren: a raspy oscillator swept between two pitches by a slow ramp, held for as long as you want.

Our own design after the classic two-transistor boxes, with the same four front-panel controls, the two ends of the pitch and the two halves of the sweep, and the switches those boxes carry on the side. Hold the Hold button, hold a pad mapped to it, or hold any note on a MIDI cord; every press starts the sweep from its beginning and release lets go over the Release time. The output is mono. The meter rides the sweep, and the sweep and the gate are also control values, so a Siren can drive a light or a Filter as it wails.

## Parameters

**PitchLow** The bottom of the sweep in Hz. Set it above PitchHigh and the sweep runs the other way.

**PitchHigh** The top of the sweep in Hz.

**LfoUp** How long the ramp takes to climb, in seconds. Unequal Up and Down give the lopsided wail.

**LfoDown** How long the ramp takes to fall, in seconds.

**Mode** Sweep climbs and falls. Rise climbs and snaps back, then rests for the Down time. Fall drops and snaps up, resting for the Up time. Step jumps between the two pitches, the two-tone.

**Wave** Square, Pulse or Saw for the oscillator. Square is the box.

**Tone** A low-pass on the way out. Turn it down for a duller horn.

**Auto** On, the ramp runs itself. Off, Manual sets the pitch by hand.

**Manual** Where between PitchLow and PitchHigh the pitch sits while Auto is off, so a mapped knob wails by hand.

**Hold** Sounds the siren while it is down.

**Release** How fast the sound dies once you let go, in milliseconds.

**Level** Output gain.

## Recipe

**Two-tone horn** Mode Step, Wave Square, PitchLow 400, PitchHigh 530, LfoUp 0.4, LfoDown 0.4, Tone 3000. Map Hold to a pad, cord the output through a Fern with a short synced repeat, and press for two bars at the top of a drop.

## Related Organisms

Kick, LFO, Button
