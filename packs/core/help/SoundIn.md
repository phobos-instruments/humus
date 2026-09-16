# SoundIn

A live stereo feed from the audio interface, or a sound file played in its place.

Drop one in, choose the input pair, and a microphone, instrument or line signal flows straight into the patch; cord its outlets to a Gain, a Fern or the Mixer. The Input dropdown lists the interface's own input names, and the meter shows what arrives before Gain. With Live input off it plays the sound file instead, which is also how a patch renders offline: with no live input available, the file is used. A mono interface fills both outlets with its single input. Drop several SoundIns pointed at different pairs to treat each pair of a multichannel interface separately. Older patches with numbered auxiliary inputs still load, each carrying its channel as a parameter.

## Parameters

**Channel** Which device input pair this SoundIn reads, shown as Input; 1/2 is the main pair. Changing it applies live.

**UseADC** Live input. On carries the interface input; off plays the File below instead.

**File** The sound file used while Live input is off. It loads as soon as it is set, and a mono file fills both outlets.

**Loop** Repeats the file when it reaches the end. Off, the file plays once and the outlets then go silent.

**Gain** A clean input trim, unity by default. Turn it down to tame a hot interface, up to lift a quiet source; the meter reads the level before it.

## Recipe

**Microphone through the patch** Set Input to the pair your microphone is on, leave Live input on, and cord the outlets into a Gain and on to a Fern before the Mixer. Set Gain so the meter sits well below the top on your loudest phrase, then load a File and switch Live input off to keep working on the patch without the microphone.

## Related Organisms

SoundOut, FilePlayer
