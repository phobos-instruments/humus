# AudioOut

A single live output channel of your audio interface, as a patchable sink.

## Channel

Which interface output this organism feeds - the dropdown lists the open device's own channel names (as your system shows them). Humus opens the addressed channel automatically when the device has it; a channel the device doesn't have swallows the signal silently.

Use AudioOut for monitor sends, headphone cues, surround stems - anything beyond the master mix. SoundOut remains the master output (the main stereo pair with the master level and mix recorder attached).

Retro-compatibility: older patches use numbered classes (AuxOut1 to AuxOut8, mapped sequentially from channel 3 up). These still load - each is this same organism with its Channel preset accordingly.

## Related Organisms

AudioIn, SoundIn, SoundOut
