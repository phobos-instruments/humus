# Lumen

A shader compositor that paints the audio it hears and stacks video layers under it.

Cord audio into its two inlets and the scene draws to it; cord its video outlet into a VideoOut to see it, the way a synth needs a SoundOut to be heard. Scene takes a fragment shader file and reloads it whenever the file changes on disk; empty runs the built-in scene. Shaders read the audio as level, bands, beat, tempo, a waveform texture and a spectrum texture, and a scene's own float inputs ride Knob1 to Knob4. The four video inlets take VideoPlayer, VideoPad, CameraIn or VideoTrack layers, stacked bottom to top, with the scene floating over them by SceneOpacity and SceneBlend. Each layer's opacity and blend live on the organism that feeds it.

## Parameters

**Scene** The shader file to run: .frag, .fs, .glsl or .fsh. Empty runs the built-in scene, and edits to the file reload while it plays.

**Brightness** The overall output level of the scene.

**Speed** How fast the scene's own clock runs. 0 freezes it; 2 runs at double speed.

**Knob1** A value the scene reads for itself; what it does depends on the shader. An effect shader takes its strength here.

**Knob2** The second such value. An effect shader takes speed and colour here.

**Knob3** The third. An effect shader takes frequency, density and grid counts here.

**Knob4** The fourth. An effect shader takes width and detail here.

**SceneOpacity** How strongly the scene shows over the video layers beneath it.

**SceneBlend** Over lays the scene on top of the video layers. Add sums it with them, so the dark parts of the scene vanish and the bright parts glow.

## Recipe

**Audio-reactive overlay** Cord the master mix into both audio inlets and a VideoPlayer into the first video inlet. Leave Scene empty, set SceneBlend to Add and SceneOpacity to 0.5, then route a Follower from the kick onto Brightness so the scene flashes with the beat. Cord the outlet into a VideoOut and open it from the Video Outputs menu.

## Related Organisms

VideoPlayer, VideoMix, VideoOut
