# VideoOut

The window or display where the picture leaves the patch.

The video twin of SoundOut. Cord a Lumen composite, a VideoMix, a VideoFX or a bare VideoPlayer into its inlet and open its window with the screen button beside the Screen menu, from the Video Outputs submenu, or by right-clicking the organism. As a window it floats above the patcher while you work; sent to a display it goes borderless and fullscreen there. Several VideoOut organisms are several outputs, so the show can go to a projector while a preview mix stays on your own screen.

## Parameters

**Screen** Where the picture goes. Window floats a resizable window you can move; Display 1, Display 2 or Display 3 takes over that attached display fullscreen, which is the projector setting.

**Fade** The master fader for the whole output. 0 is black; ride it from automation or a controller to fade the show out.

**Preview** Keeps the small preview in the organism's editor live while the output window is closed. Turn it off to spare the graphics work when you do not need it.

## Recipe

**Projector plus preview** Cord the Lumen outlet into two VideoOut organisms. Set one to Display 2 for the projector and leave the other on Window on your own screen with Preview on. Map Fade on the projector output to a fader so you can take the room to black without touching the mix.

## Related Organisms

Lumen, VideoMix, VideoPlayer
