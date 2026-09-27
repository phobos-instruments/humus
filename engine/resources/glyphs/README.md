# Icons

Drop an `.svg` (or `.png`) here to replace one of the app's drawn icons. The
file is named after the glyph it replaces, and the name is the enum member in
`engine/src/gui/style/IconGlyph.h`:

```
Play.svg   Stop.svg   Record.svg   Record-active.svg   Dice.svg   ...
```

The enum in that header is the list of what you can override.

The five edit tools ship here as examples - `Pointer.svg`, `Pencil.svg`,
`Line.svg`, `Scissors.svg`, `Eraser.svg` - each a copy of what the app draws
for itself. Edit one, or swap in your own, to change the tool strip and the
mouse cursor that goes with it. Delete a file and the drawn icon comes back.

Draw on a square canvas, in black, with no background. The app recolours the
whole drawing to whatever the theme asks for, so a multi-coloured icon comes
out flat. `Record` takes a second file, `Record-active.svg`, because it is the
one glyph whose shape changes with state (a ring at rest, a disc when armed).

The build compiles this folder into the binary beside the logo and the tour
copy, so a file here beats the drawn version and there is no icon folder to
find at runtime. Rebuild to see a change.
