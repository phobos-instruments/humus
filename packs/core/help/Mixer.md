# Mixer

Sums several signals into one, with Gain, Mute and Solo per input and a master gain and mute.

Cord your sources into the numbered inlets and the outlet on to a SoundOut or a Bus. The Inputs dropdown at the top sets how many inputs it has, 2 to 8, and Mode chooses Stereo pairs, Mono channels or Pan inputs; changing either resizes the mixer in place, keeping its name and its cords. In Stereo mode the left inlet of a pair doubles as a mono jack: one cord into the left side feeds both sides of the mix, so a mono source sits in the centre, and cording the right side as well restores true stereo. Pan mode gives each mono input its own Pan knob, a balance law that leaves the centre at full level and only turns the far side down.

## Parameters

**MasterGain** Output level after the input gains, up to twice unity.

**MasterMute** Silences the whole mix.

**Gain_1-2** Level of input pair 1-2, and so on for 3-4 up to 7-8. Unity at the top; the strip's meter shows what arrives before it.

**Mute_1-2** Silences input 1-2 without a click, and so on for the other pairs. A muted input stays silent even when soloed.

**Solo_1-2** Hears input 1-2 on its own, and so on for the other pairs. While any input is soloed, only soloed inputs are heard.

## Recipe

**Four-source mix** Set Inputs to 4 and Mode to Stereo, cord a drum organism, a bass, a synth and a Fern return into pairs 1-2 to 7-8, and cord the outlet to the SoundOut. Pull each Gain down until the SoundOut meter keeps some headroom on the loudest bar, solo one input at a time to check each part, and set the overall level with MasterGain.

## Related Organisms

Bus, Gain, Console
