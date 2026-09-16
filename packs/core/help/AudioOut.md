# AudioOut

One live output channel of the audio interface, as a mono sink.

Use it for a monitor send, a headphone cue, a click for a drummer or one stem of a surround feed: anything beyond the master mix. SoundOut remains the master output with the master level and the mix recorder attached; AudioOut takes one signal and hands it to one channel. Older patches place it as AuxOut1 to AuxOut8, which load as this organism with Channel preset from 3 upward. A channel the device does not have swallows the signal silently. Cord a Gain or a Send in front of it to set the level.

## Parameters

**Channel** Which interface output this organism feeds. The dropdown lists the open device's own channel names.

## Recipe

**Headphone cue** Put a Send on the part the performer needs to hear, cord its send pair through a Gain into an AudioOut, and set Channel to the output the headphone amplifier is on. The main mix on SoundOut is untouched, and the cue level rides the Gain.

## Related Organisms

AudioIn, SoundIn, SoundOut
