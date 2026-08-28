# Math

One line of arithmetic as a patchable object: type a formula and it runs once per sample. The two inlets arrive as a and b, the four knobs as x, y, z and w, the MIDI inlet as note, freq, gate and vel, and the clock lends t (seconds), beat, bpm and sr. Writing is playing - a draft that parses sounds immediately, and a half-typed edit keeps the last good expression running. The output is exactly what the expression says; to chain like a Number, start with "a +".

## What it is for

Four habits cover most of it. As a modulator it is an LFO you can bend: write sin(tau*beat)*x and map the out value onto any knob from the right-click Modulate with menu. As a processor it shapes whatever arrives at a - tanh(a*(1+x*15)) is a drive, floor(a*8)/8 a crush, a*(sqr(beat*2)>0) a gate. With both inlets it becomes signal glue: a*clamp(1-abs(b)*8, 0, 1) ducks a under b. And as a voice it is a bare oscillator - sin(tau*ph(freq)) - happiest as sirens, zaps and drones. The presets keep one of each habit; the ones marked (a) shape the inlet and say nothing until something is corded in. Every pitched preset takes its pitch from freq, so Freq or a held key plays all of them and the knobs are left to character. The dice on the property box writes a fresh formula with new knob settings under it.

## Pitch that moves

ph(hz) is a phase that runs at hz and wraps 0..1, so sin(tau*ph(110 + z*880)) sweeps cleanly however fast z moves. Writing the same thing as sin(tau*t*(110 + z*880)) clicks: the phase there is t times the frequency, and every change in frequency jumps it by an amount that grows the longer the patch has been running. Reach for ph whenever a knob, an inlet or a note sets the pitch; t is for slow sweeps and envelopes. The knobs themselves glide over a few milliseconds, so a turn or a modulation lands without steps.

## Harmonics

harm(hz, n, tilt) stacks n harmonics of hz in one call, each one 1/k^tilt as loud as the fundamental: tilt 0 is a buzz with every harmonic equal, 1 leans saw, 2 is rounder, and anything above Nyquist is left out so it never aliases. The stack is as loud as one sine whatever n is, and n can be fractional - the top harmonic fades in with the fraction, so a knob sweeping the count never pops. harm(freq, 1 + x*15, y*2) is an additive voice with x counting the harmonics and y shaping them - the Harmonics preset. Prism is for material whose pitch has to be found first; a tone you generate here already knows its pitch, so build the harmonics in the formula.

## The vocabulary

Numbers, parentheses, + - * / %, comparisons that answer 0 or 1, and if(c, then, else). Putting things next to each other multiplies, as on paper: 2a, 2pi and x(y+1) all work. Functions: sin cos tan tanh asin exp log log2 sqrt abs floor ceil fract pow min max clamp mix step smoothstep, the phase shapes saw tri sqr pulse (feed them 0..1 phase, they answer -1..1), ph(hz), harm(hz, n, tilt), env(gate, attack, release) and noise(). The constants pi, tau and e are spoken for. prev is the previous output sample - one-sample feedback for slews and little filters: mix(prev, a, 0.1) is a one-pole smooth.

## Played from a cord

Cord a keyboard, a PianoRoll or a DNA into the MIDI inlet and the expression sees the note it holds: note (the MIDI number), freq (its pitch in Hz, following the patch Tuning), gate (1 while a key is down) and vel (0..1). Hold a chord and the last key pressed wins. With Auto on the box plays on its own: freq is the Freq knob, gate reads 1 and vel 1, until a key is actually down. Flip Auto off and it sounds only while a key is held, so sin(tau*ph(freq)) * vel * env(gate, 0.01, 0.3) is a playable voice - env follows gate with an attack and a release in seconds, and holds its own state, which prev (the previous output sample) cannot do inside a product.

## Stereo

There are two outlets. ch is 0 on the left one and 1 on the right: a line that never mentions ch is worked out once and sent to both, and a line that does is worked out per side, each with its own phases and envelopes. harm(freq*(1 + (ch*2-1)*0.005), 8, 1) is a detuned pair - the Wide preset - and if(ch, b, a) puts the inlets left and right.

A formula is saved as written, parsing or not. If the tail of the line was never finished, the box plays the longest part that parses.

## Time and the clock

Rolling, t and beat ride the transport and jump with it on a seek. Stopped, they free-run from where they were, so an oscillator keeps sounding and a mapped wobble keeps waving without the timeline moving. The plot under the formula names what the line is: a voice when it runs an oscillator or answers the MIDI inlet, EFFECT when it works on a or b, a modulator otherwise. An effect is drawn working on a probe tone that shows faintly behind the result, so a ring mod or a drive has a shape before anything is corded in. The window is four cycles of freq for a voice or an untimed effect, one beat for anything that moves with the clock. The out control value carries the last sample of every block, scaled 0..1: it follows control-rate expressions faithfully, while audio-rate material is better tamed with a Follower on the outlet.

Nothing in the field can hurt the patch. Division by zero answers 0, wild results are clamped, and an edit that does not parse points at the offending column while the last good expression plays on.

## Parameters

**Expression** the formula. a and b are the inlets; x, y, z and w the knobs; ch is the outlet side.

**X** knob variable x, 0 to 1. Automate it, map it, morph it.

**Y** knob variable y, 0 to 1.

**Z** knob variable z, 0 to 1.

**W** knob variable w, 0 to 1.

**Auto** on, the box plays by itself at Freq; off, only while a MIDI note is held, at its pitch.

**Freq** the pitch freq carries while Auto is on, 20 to 2000 Hz.

## Related Organisms

Number, LFO, Follower, VCA, Wave, Microdot
