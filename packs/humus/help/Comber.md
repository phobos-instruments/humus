# Comber

A bank of tuned comb filters that draws chords out of whatever passes through it.

A comb filter is a delay so short, with feedback, that it rings at a pitch instead of echoing. Comber sets up to eight of them side by side, five to begin with, each with its own pitch, decay and level, so drums, noise or a voice come out ringing at the notes the combs are tuned to. Every comb shows its pitch three ways: a slider, the number in hertz, and the nearest note name with the cents it is off by. Click the note name to pick a note from a keyboard, or roll the wheel over it to step by semitones. The combs ring side by side and are summed, so what comes out is a chord. Moving a pitch glides instead of clicking, and it runs in stereo.

## Parameters

**InputGain** The master level, applied as the sound goes in. Lower it when long decays start to pile up.

**Combs** How many combs are in use, one to eight. The rows beyond it are hidden and silent.

**Frequency_1** Pitch of comb 1 in hertz, 16 to 12000, and so on for the others. The delay is one period of this frequency.

**DecayTime_1** How long comb 1 rings after the input stops, 0.02 to 10 seconds, and so on for the others. Short is a pitched slap, long is a held note.

**Gain_1** Level of comb 1, and so on for the others: the comb's share of the sum, so zero silences it.

## Recipe

**Chord from a drum loop** Cord a drum machine or a loop in. Tune the five combs to 110, 165, 220, 277 and 330, set every decay to 2, every comb gain to 0.6 and InputGain to 0.5. Shorten the decays toward 0.3 for a pitched slap, or push one comb to 8 for a note that hangs over the rest.

## Related Organisms

Fern, Verbatim, ParaEQ
