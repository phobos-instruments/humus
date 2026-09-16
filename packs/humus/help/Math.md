# Math

A one-line formula that runs once per sample, as an oscillator, a processor or a modulator.

Volume down first: Math plays whatever you type straight to the outlet, and a formula can be full scale, pure DC or a tone far outside hearing. The two inlets arrive as a and b, the four knobs as x, y, z and w, the MIDI inlet as note, freq, gate and vel, and the clock lends t, beat, bpm and sr. A draft that parses sounds immediately and a half-typed edit keeps the last good expression running. There are two outlets: a line that never mentions ch is sent to both, and one that does is worked out per side. The out control value follows the last sample of each block, so Control with maps the formula onto any knob. Cord a PianoRoll into the MIDI inlet for a voice, or an audio signal into a for a processor.

## The vocabulary

Numbers, parentheses, + - * / %, comparisons that answer 0 or 1, if(c, then, else); adjacent terms multiply, so 2a works. Functions: sin cos tan tanh asin exp log log2 sqrt abs floor ceil fract pow min max clamp mix step smoothstep, the phase shapes saw tri sqr pulse, ph(hz), harm(hz, n, tilt), env(gate, attack, release) and noise(). prev is the previous output sample, dt is one sample in seconds, and pi, tau and e are set. Statements separate with semicolons, and name := value keeps a value between samples.

## Parameters

**Expression** The formula. a and b are the inlets, x, y, z and w the knobs, ch the outlet side. It is saved as written, parsing or not.

**X** Knob variable x, 0 to 1. Automate it, map it or morph it.

**Y** Knob variable y, 0 to 1.

**Z** Knob variable z, 0 to 1.

**W** Knob variable w, 0 to 1.

**Auto** On, the formula plays by itself with freq at the Freq knob and gate at 1. Off, it sounds only while a MIDI note is held, at that note's pitch.

**Freq** The pitch freq carries while Auto is on and no key is down, 20 to 2000 Hz.

## Recipe

**Playable voice** Auto off, Expression sin(tau*ph(freq)) * vel * env(gate, 0.01, 0.3), and cord a PianoRoll into the MIDI inlet. Replace sin(tau*ph(freq)) with harm(freq, 1 + x*15, y*2) and x counts the harmonics while y shapes them.

## Related Organisms

Number, LFO, Follower, Wave
