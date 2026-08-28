# Chorus

One voice into several. A copy of the sound is delayed by a few milliseconds and that delay is kept gently moving, so the copy drifts slightly sharp and slightly flat around the original. Two players never land on exactly the same pitch at exactly the same moment either, and that failure to agree is what makes a section sound bigger than a soloist. This is that failure, arranged on purpose - the gentlest way to make something sound larger without making it louder.

## The family

A chorus, a flanger and a phaser are the same idea at three scales. Chorus delays long enough to hear as detuning; Flanger uses much shorter delays, so the copies comb rather than detune; Phaser does not delay at all but shifts phase. Chorus is the one that thickens, the other two sweep.

## Parameters

**MinDelay** the shortest the delay ever gets, the floor the sweep sits on. Small values drift toward flanging; larger ones stay clearly chorus.

**DelaySweepDepth** how far the delay travels from that floor. More depth is more detuning, and past a point it goes from richness to seasickness.

**Rate** how fast the delay travels. Slow is a swell you feel rather than hear; fast is vibrato.

**RightLFOPhaseOffset** how far behind the left the right channel's sweep runs, in degrees. At zero both sides detune together and the result stays centred. At 90 or 180 they disagree, and the disagreement opens the image out.

**Stereoness** how wide the delayed copies are panned. Past 100 it uses phase inversion between the channels to push the sound outside the speakers - striking, but check it in mono first, because that is exactly where it will partly cancel.

**Feedback** returns the delayed copy to the input. Small amounts add resonance. It takes a negative value too, which inverts on the way round and hollows the sound instead of thickening it.

**HFRolloffFrequency** darkens the delayed copies from above. Keeping the wet path duller than the dry one is what stops a chorus sounding glassy.

**WetDryMix** how much of the delayed sound is heard against the original. A chorus at full wet is not a chorus any more, because the detuning is only audible against something steady.

## How it works

The sweep is a triangle, and each channel gets its own, offset by RightLFOPhaseOffset. Because a moving delay resamples what is stored in it, the pitch of the copy rises while the delay shortens and falls while it lengthens: the detuning is not added afterwards, it falls out of the movement itself. That is also why Rate and DelaySweepDepth interact - it is the speed of the change that sets how far the pitch bends, so doubling either bends it further.

This organism is stereo only.

## Related Organisms

Flanger, Phaser, Fern, StereoTool
