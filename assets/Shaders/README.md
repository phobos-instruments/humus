# Shaders

The GLSL scenes Humus ships, loaded by the Lumen organism. A `.frag` or `.fs`
here is a single-pass scene; multi-pass scenes are `.synScene` folders.

These live inside the application. Your own belong in
`Documents/Humus/assets/shaders/`, which Humus makes for you on first run:
anything you drop there appears alongside these and wins if it has the same
name. Nothing here is ever copied there, so an update can improve these
without touching yours.

A patch refers to one by name - `asset:shaders/breathe.frag`, not a path - so
a patch using a shipped scene opens on anyone's computer.
