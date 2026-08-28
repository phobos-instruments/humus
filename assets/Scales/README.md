# Scales

The tunings Humus ships, in Scala `.scl` format, read by the Tuning organism.
They are grouped by folder, and the folder name is what the browser shows
beside each scale.

These live inside the application. Your own belong in
`Documents/Humus/assets/scales/`, which Humus makes for you on first run:
anything you drop there appears alongside these and wins if it has the same
name. Nothing here is ever copied there, so an update can improve these
without touching yours.

A patch refers to one by name - `asset:scales/Just Intonation/just-major.scl`,
not a path - so a patch using a shipped scale opens on anyone's computer.
