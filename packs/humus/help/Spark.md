# Spark

A riser that never arrives. Twelve micro-delay lines, six per channel, each only milliseconds long and each mixed back against the dry signal so it rings as a comb filter. Every line sweeps its comb through the same span of octaves, but each one starts at a different point in that sweep, and each fades in at the bottom and out at the top under its own gain window. Six windows evenly spread sum to a constant, so nothing seams: the pitch climbs forever without ever leaving.

## The rise

Mode sets which way the lines travel. Rise climbs, Fall descends, and Free rocks back and forth around the middle of the span instead of wrapping. Rate is how fast one line crosses the whole span - a full sweep every eight seconds at the default, slower for a build that takes a whole section. Time is the shortest delay a line reaches, the top of its climb; Range is how many octaves below that the line starts. Together they set where the comb lives: short times sit up in the whistle, longer ones drop into a fluttering body.

## The spark

Feedback is how long each comb rings. Low, the lines colour the sound and pass on; high, each one becomes a narrow resonant tooth with a pitch of its own. Damp rolls the top off inside that loop, so long tails go warm instead of shrill.

Spark is the strange one. At zero the lines are spread evenly and the rise is smooth. Turn it up and the spacing bunches: lines land on top of each other, their teeth reinforce, and notes appear that are in none of the lines on their own - the same way two ring patterns crossing on water throw up a splash where they meet. The offsets are fixed, so a given Spark setting always gives back the same notes.

## The panel

Spread offsets the right channel from the left, so the two sides climb out of step and the rise widens as it goes. Mix blends against the dry input - the comb wants both halves, so the middle is where it lives - and Level sets the output.

The dice rolls Rate, Time, Range, Feedback, Damp, Spark and Spread; Mode, Mix and Level stay where you put them.

## Parameters

**Mode** Rise, Fall or Free.

**Rate** how fast one line crosses the span, in hertz.

**Time** shortest delay reached, in milliseconds.

**Range** octaves the sweep covers below Time.

**Feedback** how long each comb rings.

**Damp** top-end roll-off inside the feedback loop.

**Spark** phase bunching. 0 is an even spread; up is phantom notes.

**Spread** right-channel offset from the left.

**Mix** dry to combed blend.

**Level** output level.

## Signal flow

Input -> six delay lines per channel, each swept and windowed, each with its own damped feedback -> summed -> Mix against dry -> Level -> Output. Stereo in, stereo out.

## Related Organisms

Fern, Flanger, SChorus
