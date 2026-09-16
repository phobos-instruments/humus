# Microdot

A rolling offbeat bass synth driven by a sixteen-step trigger grid locked to the transport.

Every hit is a band-limited oscillator with an optional sub-octave square through a fast filter envelope and a drive stage. A fresh organism is seeded with the classic roll, three sixteenths after each downbeat, so it locks in as soon as the transport plays, and it is silent while stopped. The grid is always the gate; Seq decides who owns the pitch. With Seq on every hit plays the Note field. With Seq off the hits follow whatever the MIDI inlet is holding, so a Riff, a Steps or a PianoRoll corded in has its line re-chopped by the grid. Pair it with a Kick in 4/4 and cord both into a Compressor.

## The grid

Click a step to toggle it; the column the transport is on is lit. The ^ row lifts that step's hit one octave, and the hold row at the bottom stretches the previous note through that step, so a run of them holds one note. Right-click for the stamps: Roll -111, Offbeat --1-, Full 1111, Random and Clear. The pattern is stored in the patch.

## Parameters

**Note** The pitch every hit plays while Seq is on.

**Wave** SAW, SQR, PLS (a thin pulse), TRI, SIN or FAT, a detuned saw pair for width.

**Seq** On, the Note field owns the pitch; off, the held MIDI note does. The grid gates either way.

**Cutoff** The filter's resting frequency.

**Reso** Resonance. Raise it and the envelope sweep starts to sing.

**EnvMod** How far the envelope opens the filter above Cutoff on each hit.

**Decay** Length of the filter envelope in milliseconds; it shapes the note's level as well. 80 to 150 keeps the tight rolling character.

**Gate** Hit length as a fraction of the step. Shorter is bouncier.

**Sub** Sub-octave square underneath, for weight.

**Drive** Saturation after the filter.

**Level** Output level.

**SwingFollow** Follow: takes the patch groove from the transport instead of the Swing slider.

**Swing** Delays every second sixteenth, up to a triplet feel. Used only while Follow is off.

## Recipe

**Rolling bass** Note A1, Wave SAW, Cutoff 700, Reso 0.35, EnvMod 0.5, Decay 120, Gate 0.8, Sub 0.4, Drive 0.35, Follow on. Add a Kick with 4/4 on and press play; the kick lands on the beat and the bass rolls between. Set Seq off and cord a Riff in to give the roll a moving line.

## Related Organisms

Kick, Acid, Riff, DNA
