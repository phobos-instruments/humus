# Acid

A mono line synth in the acid-box tradition, with accent and slide.

A band-limited saw or square runs into a resonant diode ladder that an exponential envelope sweeps open on every note. Two things make its lines speak: a note at velocity 110 or above is an accent, which opens the filter further and hits louder, and two overlapping notes slide in pitch without restarting the envelope. The ladder and the output stages that shape its low end are our own version of the ones in Open303 by Robin Schmidt. Cord a Riff, a Steps or a PianoRoll into its MIDI inlet, and with its editor focused the computer keyboard plays it directly.

## Parameters

**Wave** Oscillator shape: SAW, SQR, PLS or TRI.

**Cutoff** The filter's base frequency. The envelope sweeps up from here.

**Resonance** Filter emphasis, from a gentle roll-off to the full squelch.

**EnvMod** How far the envelope sweeps the cutoff above Cutoff.

**Decay** How fast the filter sweep falls back, in milliseconds.

**Accent** How much harder an accented note hits. Zero makes every note plain.

**Glide** How long a slide between overlapping notes takes to arrive, in milliseconds. 60 is the classic setting.

**Attack** How quickly the volume rises at the start of a note, in milliseconds.

**AmpDecay** How fast a held note falls towards Sustain. At the top of its range the note does not fall at all.

**AccentDecay** The filter decay used on accented notes, in milliseconds, so accents can ring longer or snap shorter than plain notes.

**FilterFM** Feeds the output back into the cutoff, so the filter modulates itself. Zero is clean, higher is rougher.

**Track** How far the cutoff follows the played pitch. Zero keeps the filter fixed whatever the note.

**Sustain** Where a held note settles after AmpDecay. Zero is the box, where every note dies away; up, the note holds until it is released.

**Squelch** How much low end the filter's feedback is allowed to remove. Low keeps the bass under heavy resonance; high thins it into the classic squelch.

**Velocity** How much velocity shapes the accent. At zero an accent is all or nothing above velocity 110; at one every velocity sets its own accent amount.

**Punch** The filter envelope also lifts the volume, more so on accents. Zero turns that off.

**Muffler** A low-pass on the output, to tame the top when Drive is up.

**Drive** Saturation on the way out.

**LfoWave** The shape of an extra wobble on the cutoff: SIN, TRI, SQR, SAW, DSAW or S&H.

**LfoRate** The wobble's speed in Hz while LfoSync is off.

**LfoDepth** How far the wobble moves the cutoff. Zero switches it off.

**LfoSync** Locks the wobble to the transport instead of LfoRate.

**LfoBeats** The length of one wobble cycle in beats while LfoSync is on.

**Level** Output volume.

**BendRange** How far the pitch wheel reaches at full travel, in semitones. Two is the common default; zero ignores the wheel.

## Recipe

**Squelch line** Wave SAW, Cutoff 400, Resonance 8, EnvMod 0.7, Decay 250, Accent 0.6, Glide 60, Drive 0.4. Cord a Riff in with a few accented steps and one slide, then ride Cutoff by hand over the bar.

## Related Organisms

Riff, Steps, Microdot
