# CameraIn

A camera as a video source.

Turn Enabled on and cord the video outlet into a Lumen layer, a VideoMix, a VideoFX or a VideoOut, and the live picture becomes a layer like any tape. The camera opens only while Enabled is on and closes when it goes off; the frames go to the display and nowhere else. It is a picture, not a controller: to play something with a camera, cord this into Hands or Skeleton, or let those open the camera themselves.

## Parameters

**Enabled** Opens the camera. Off closes the device rather than hiding the picture.

**Camera** Which capture device to open, when the machine has more than one.

**Mirror** Flips the picture left to right, so leaning left moves you left on screen. Turn it on when you perform in front of the camera.

**Opacity** How strongly this layer shows over the layers beneath it in a Lumen.

**Blend** How the layer combines with those beneath it: Normal, Add, Multiply or Screen.

## Recipe

**Camera under a scene** Enabled on, Mirror on, then cord the video outlet into Lumen's first video inlet and Lumen into a VideoOut. Load a scene shader in Lumen, set SceneBlend to Add and SceneOpacity to about 0.6, so the scene glows over the live picture.

## Related Organisms

Lumen, VideoMix, VideoOut, Hands
