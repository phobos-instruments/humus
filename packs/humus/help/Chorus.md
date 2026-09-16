# Chorus

A stereo chorus that thickens a sound with a slowly moving delayed copy.

A copy of the input is delayed by a few milliseconds and that delay is kept moving, so the copy drifts slightly sharp and flat around the original, the way two players never land on exactly the same pitch. The sweep is a triangle, and the right channel's sweep runs behind the left's by RightLFOPhaseOffset, which is what opens the image. Rate and DelaySweepDepth interact: the speed of the delay change sets how far the pitch bends, so doubling either bends it further. Cord it after a Rhizome, a Wave or a Sampler; a Flanger is the same idea at shorter delays, a Phaser the same idea with no delay at all.

## Parameters

**MinDelay** The shortest the delay ever gets, in milliseconds. Small values drift towards flanging, larger ones stay clearly chorus.

**DelaySweepDepth** How far the delay travels above MinDelay, in milliseconds. More depth is more detuning.

**Rate** How fast the delay travels, in Hz. Slow is a swell, fast is vibrato.

**RightLFOPhaseOffset** How far behind the left the right channel's sweep runs, in degrees. At zero both sides detune together and the result stays centred; at 90 or 180 they disagree and the image opens.

**Feedback** Returns the delayed copy to the input. Small amounts add resonance; a negative value inverts on the way round and hollows the sound instead of thickening it.

**HFRolloffFrequency** Darkens the delayed copy from above, in Hz. Keeping the wet path duller than the dry one stops a chorus sounding glassy.

**Stereoness** Reserved for the width of the wet image. The current chorus keeps each side's copy on its own side, so this knob has no audible effect yet.

**WetDryMix** How much of the delayed sound is heard against the original. At full wet the detuning has nothing steady to be heard against.

## Recipe

**Wide pad** MinDelay 8, DelaySweepDepth 6, Rate 0.3, RightLFOPhaseOffset 180, Feedback 0, HFRolloffFrequency 8000, WetDryMix 0.5. Cord a sustained Rhizome chord in and the two sides drift apart around the centre.

## Related Organisms

Flanger, Phaser, Fern, StereoTool
