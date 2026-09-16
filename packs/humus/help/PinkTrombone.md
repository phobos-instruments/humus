# PinkTrombone

A human voice built from a model of the throat, after Pink Trombone by Neil Thapen.

There is no sample and no oscillator bank: a model of the glottis makes the buzz of the vocal folds, and a waveguide the shape of a throat, mouth and nose filters it into vowels, hums and hisses. Play it from MIDI: the held note is the pitch and velocity leans into the loudness. The Tongue pair steers the vowel, Squeeze pinches the tract shut at Place for fricatives and stops, and Nasal opens the passage to the nose. Small wanders in pitch and pressure keep a held note alive. Cord an LFO or a Slider onto TongueX, Squeeze and Nasal and it babbles; cord its output into a Harmonizer or a Trellis.

## Parameters

**TongueX** Front. Slides the tongue's hump between throat and teeth; with TongueY it places the vowel.

**TongueY** Raise. Arches the tongue toward the palate.

**Place** Where the tract is pinched: lips at the top of the dial, throat at the bottom.

**Squeeze** How hard the tract is pinched at Place. None for vowels, a little for fricatives, all the way to stop the air so that releasing it pops a plosive.

**Nasal** Opens the velum so sound escapes through the nose. Hold it open for m and n hums, or colour a vowel with a little of it.

**Tenseness** Vocal fold pressure, from breathy whisper to pressed buzz. Velocity scales the result.

**Vibrato** Depth of the deliberate pitch wave and of the slow drift under it.

**Wobble** Exaggerates the involuntary wander into a slow warble.

**Drone** Keeps the voice sounding with no note held, at the last pitch it was given.

**Level** Output gain.

**BendRange** How far the pitch wheel reaches at full travel, in semitones. Two is the common default; zero ignores the wheel.

## Recipe

**Choir bed** Drone on, Tenseness 0.5, Vibrato 0.3, TongueX 0.3 and TongueY 0.6 for an oo. Send one MIDI note to set the pitch, cord an LFO at 0.1 Hz onto TongueX at a small depth so the vowel drifts, and cord the output through a Verbatim.

## Related Organisms

Harmonizer, Trellis, Decomposer
