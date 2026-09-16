# RNG

A source of random numbers drawn from the machine's own entropy, sent as a control value.

Every draw comes from system entropy rather than a formula, so the sequence cannot be replayed or guessed ahead. The outlet carries the latest draw, which rides a control cord onto any socket in the patch or into a Number to be read and passed on. Onto a knob, a draw made in a width reads as a proportion of that width, so 200 of 255 is most of the way up; onto a Number it shows as the number itself. The readout along the bottom shows the draw and the lamp beside Rate flashes on each one. Draws happen on a clock, on the transport's beat, or on demand from Trigger.

## Parameters

**Rate** Draws per second while Sync is off, up to 50. Zero stops the clock so only Trigger draws.

**Sync** Takes the draw clock from the transport instead of Rate, so draws land with the music and follow the tempo.

**SyncBeats** Musical time between draws while Sync is on: a quarter of a beat for something restless, four for one change a bar.

**Format** What kind of number a draw is. 0 to 1 is the plain unit value, Coin is that rounded to nought or one, and 8-bit, 8-bit signed, 16-bit and 16-bit signed draw a whole number across that width.

**Trigger** Draws one number on a rising edge, so a Button, a note or a sensor can ask for a number instead of waiting for the clock.

## Recipe

**New cutoff every bar** Sync on, SyncBeats 4, Format 0 to 1. Cord the outlet onto the Frequency socket of a Filter placed after a Drums organism and set the route's range from 300 to 3000; the cutoff jumps to a new place at the top of every bar.

## Related Organisms

Number, Slider, LFO, Steps
