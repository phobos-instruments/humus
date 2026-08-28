# VideoOut

Where the picture leaves the patch - the video twin of SoundOut. Cord a composition into its video inlet (a Lumen's composite, a VideoMix, a bare VideoPlayer deck) and open its window from Control > Video Outputs, or by right-clicking the organism. The window stays above the patcher while you work the knobs; sent to a display it detaches and nothing can cover it.

Screen sends it out: Window floats a resizable window; Display 1-3 goes borderless fullscreen on that display - the projector setting. Fade is the master fader: 0 is black, and being an ordinary param it rides automation, MIDI maps and modulation routes - fade the whole show to black on a filter sweep.

Several VideoOut organisms are several outputs: send the composition to the projector and a preview mix to your own screen at once. Decks and mixers also preview their own feed via right-click > Open Visuals, VJ-style; a Lumen's composite you watch through its VideoOut.

## Parameters

**Screen** where the picture goes: a window you can move, or one of the attached displays, which it takes over fullscreen.

**Fade** the master fader for the whole output. 0 is black.

**Preview** keeps the small preview in the organism's editor live even while the output window is closed. Turn it off to spare the graphics work when you do not need the picture.

## Related Organisms

Lumen, VideoMix, VideoPlayer
