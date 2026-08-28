# AudioIn

A single live input channel of your audio interface, as a patchable source.

## Channel

Which interface input this organism reads - the dropdown lists the open device's own channel names (as your system shows them). Humus opens the addressed channel automatically when the device has it; a channel the device doesn't have plays silence.

Use one AudioIn per microphone/line you want to treat separately. SoundIn remains the main stereo pair (channels 1/2) and can also stream sound files; AudioIn is the way to reach every other input on a multichannel interface.

Retro-compatibility: older patches use numbered classes (AuxIn1 to AuxIn8, mapped sequentially from channel 3 up). These still load - each is this same organism with its Channel preset accordingly.

## Related Organisms

AudioOut, SoundIn, SoundOut
