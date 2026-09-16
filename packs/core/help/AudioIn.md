# AudioIn

One live input channel of the audio interface, as a mono source.

Use it to reach any single input on a multichannel interface: one AudioIn per microphone or line you want to treat on its own. SoundIn carries the main stereo pair and can also play a file; AudioIn is the plain single-channel tap. Older patches place it as AuxIn1 to AuxIn8, which load as this organism with Channel preset from 3 upward. Cord its outlet into a Gain, a Mixer strip or straight into an effect. A channel the device does not have plays silence.

## Parameters

**Channel** Which interface input this organism reads. The dropdown lists the open device's own channel names.

## Recipe

**Second microphone** Drop an AudioIn, set Channel to the input the microphone is on, and cord it into a Mixer strip beside the SoundIn. Trim it with the strip's Gain so the two sources match before any effect hears them.

## Related Organisms

AudioOut, SoundIn, SoundOut
