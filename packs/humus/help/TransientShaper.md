# TransientShaper

Reshapes the attack and the sustain of a sound independently of how loud it is. There is no threshold: a ghost note at -60 dB and a hit at 0 dB are shaped by exactly the same amount. That is what separates this from a compressor - you are sculpting the shape of an envelope, not trading level against a threshold.

Use it to put the smack back into a soft drum loop, to pull a close-mic'd kit out of its room, or to make a synth stab bite.

## Parameters

**Mode** switches stereo and mono in place, keeping the name, settings, cords and automation. Detection is stereo-linked, so one gain is applied to both channels and the image never shifts.

**Attack** shapes the onset. Positive exaggerates the transient, for more snap and more click; negative softens it, taming a plucky attack or pushing a sound back.

**Sustain** shapes the body and tail. Positive lengthens it and pulls up the room; negative shortens it, giving the dry, tight, gated-sounding drum without a gate.

**OutputGain** makes up level after shaping.

## How it works

Two envelope followers run on the detector: a fast one that snaps onto an onset and a slow one that lags behind it. Their normalised difference, (fast - slow) / slow, is scale-free, which is why level does not matter. It goes positive during an attack, where the fast follower leads, and negative during the sustain and tail, where the slow one leads. Attack rides the positive part and Sustain the negative, so the two knobs act on genuinely different parts of the envelope instead of fighting each other.

## Usage

Start with both knobs at 0 and move one at a time. The effect is easy to overdo because it does not run out of headroom the way a compressor does, and small amounts do a lot; on a drum bus, a little positive Attack with a little negative Sustain is the classic tighter-and-punchier move. If Attack starts to click or sound brittle, back off before reaching for OutputGain - that edge is the transient being exaggerated past what the source actually has. Negative Sustain is a far more natural way to dry out a room than a gate, because nothing ever slams shut.

## Notes

Because it ignores absolute level, it will happily shape the noise floor and the bleed between hits too. On a very noisy source, gate or edit first.

## Related Organisms

SideChain, Console, Kick, Sampler
