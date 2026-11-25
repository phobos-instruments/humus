# Math

A one-line formula that runs once per sample, as an oscillator, a processor or a modulator.

Volume down first: Math plays whatever you type straight to the outlet, and a formula can be full scale, pure DC or a tone far outside hearing. The two inlets arrive as a and b, the four knobs as x, y, z and w, the MIDI inlet as note, freq, gate and vel, and the clock lends t, beat, bpm and sr. A draft that parses sounds immediately and a half-typed edit keeps the last good expression running. There are two outlets: a line that never mentions ch is sent to both, and one that does is worked out per side. The out control value follows the last sample of each block, so Control with maps the formula onto any knob. Cord a PianoRoll into the MIDI inlet for a voice, or an audio signal into a for a processor.

## The vocabulary

Numbers, parentheses, + - * / %, comparisons that answer 0 or 1, if(c, then, else); adjacent terms multiply, so 2a works. Functions: sin cos tan tanh asin exp log log2 sqrt abs floor ceil fract pow min max clamp mix step smoothstep, the phase shapes saw tri sqr pulse, ph(hz), harm(hz, n, tilt), env(gate, attack, release) and noise(). prev is the previous output sample, dt is one sample in seconds, phase is the cycle of freq running from 0 to 1, and pi, tau and e are set. Statements separate with semicolons, and name := value keeps a value between samples.

rate is the pace of whatever plays the formula: 1 in a Math, and the LFO's Rate when the formula plays in an LFO, so a motion that steps by rate*dt speeds up with it. The Nature presets are motions written out this way, to read and change: Gravity bounces a ball, Pendulum swings, Heart beats lub and dub, Breath breathes in and out, Swarm flashes like three fireflies pulling into step, Wind gusts and Wander roams like a hand on a knob, each from regular at X 0 to wild at 1. noise() follows its own sequence in each box, so two Maths wander differently while a patch always plays the same. A formula that steps by dt plots eight seconds.

## Parameters

**Expression** The formula. a and b are the inlets, x, y, z and w the knobs, ch the outlet side, and any other name is the Var of that name corded into Vars (lower-cased, spaces as underscores). It is saved as written, parsing or not.

**X** Knob variable x, 0 to 1. Automate it, map it or morph it. A Var named x corded into Vars drives this pot: the knob moves with the Var and the sound follows.

**Y** Knob variable y, 0 to 1.

**Z** Knob variable z, 0 to 1.

**W** Knob variable w, 0 to 1.

**Auto** On, the formula plays by itself with freq at the Freq knob and gate at 1. Off, it sounds only while a MIDI note is held, at that note's pitch.

**Freq** The pitch freq carries while Auto is on and no key is down, 20 to 2000 Hz.

**Vars** One inlet for every Var you want to show on the patch. Cord a Var in and its name does the rest: x, y, z and w drive their pots, any other name is read by the formula. Only corded Vars count: pull the cord and the link is gone.


## Recipe

**Playable voice** Auto off, Expression `sin(tau*ph(freq)) * vel * env(gate, 0.01, 0.3)`, and cord a PianoRoll into the MIDI inlet. Replace `sin(tau*ph(freq))` with `harm(freq, 1 + x*15, y*2)` and x counts the harmonics while y shapes them.

**Named controls** Make a Var called x and one called depth, cord both into Vars, and write `sin(tau*ph(freq)) * depth * x`. Draw a Slider or an LFO onto each Var's Value: the X pot moves with the first, the formula reads the second, up to eight names.

**Living motion** Load the Nature preset Gravity and use Control with on a filter's cutoff: the ball bounces sooner and lower until it rests, then is thrown up again. Turn X up for a livelier ball, or load Heart, Wind or Breath.

**A shape for an LFO** Write a cycle in terms of phase, such as `pow(sin(tau*phase), 3)` or `tri(phase)*x`, and cord the Math into an LFO's Function socket. The LFO switches to FN and plays the shape at its own Rate, following every edit and knob turn here.

## Related Organisms

Number, LFO, Follower, Var, Wave
