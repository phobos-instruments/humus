# Slider

A fader whose position is a control value any knob in the patch can follow.

The outlet carries the handle's position as a control value, so a knob can ride it through a control cord or a Control with route, and the inlet sets the handle from another control, so a sensor or an LFO can move it. Min and Max are the numbers the two ends stand for and the outlet declares that range, so a knob follows the position across its own travel while a Number corded to the same outlet reads the mapped value. Map a hardware fader to Value through MIDI learn and it becomes a macro steering several routes at once. Give a route a switch shape and the Slider flips a Bypass or opens a Gate past a threshold.

## Parameters

**Value** The handle, from 0 to 1. The inlet sets it and the outlet sends it.

**Min** The number the bottom of the throw stands for. Min above Max inverts the throw.

**Max** The number the top of the throw stands for.

**Slew** How long the value takes to reach a new setting, in milliseconds. At 0 it arrives at once; higher turns a jump into a glide.

**Log** Bends the travel between Min and Max logarithmically, so the low end gets more room. Use it for frequencies and times; both ends must be above zero.

## Recipe

**Filter macro** Min 80, Max 12000, Log on, Slew 60. Right-click a Filter's Cutoff and pick Control with Slider, then right-click a Fern's Feedback and do the same with a narrower range. One fader now opens the filter and lengthens the echo together; map Value to a hardware fader through MIDI learn.

## Related Organisms

Number, Sig, LFO, Gate
