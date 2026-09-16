# TransientShaper

Shapes the attack and the sustain of a sound independently of its level.

A fast and a slow envelope follower run on the input, and their normalised difference says whether the signal is in an onset or a tail regardless of how loud it is, so a quiet ghost note is shaped by the same amount as a full hit. Attack acts on the onset and Sustain on the tail, each up to 15 dB either way. Detection is stereo-linked, so the image does not shift. Bleed and noise floor are shaped too, so put a Gate before it on a noisy source. Cord a Drums organism, a Sampler loop or a Kick in.

## Parameters

**Attack** Shapes the onset. Positive exaggerates the transient for more snap; negative softens it. If it starts to click, back off here rather than reaching for OutputGain.

**Sustain** Shapes the body and tail. Positive lengthens it and pulls up the room; negative shortens it for a dry, tight sound without a gate.

**OutputGain** Makes up level after shaping.

## Recipe

**Tighter drums** Attack 0.3, Sustain -0.4, OutputGain 1 on a drum loop from a Sampler. The hits gain snap and the room between them drops away. Small moves do a lot here because the effect never runs out of headroom the way a compressor does.

## Related Organisms

SideChain, Console, Kick, Sampler
