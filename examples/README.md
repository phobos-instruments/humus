# Examples

Worked examples, reachable from **File > Examples** in Humus, which mirrors
this folder tree as menus. The tree is shipped with the app and with the
source, so a reader can open the same file from either.

The rules of the tree:

- The first level is what an example is about. `Organisms/` holds one folder
  per organism, and a folder for each scenario that organism plays. Other
  first-level sections may follow as the examples grow, a scenario that spans
  several boxes for instance.
- A scenario is a folder that holds everything it needs beside its patches:
  the sketch a board runs, a table a box reads. A sketch lives in a folder of
  its own name, `NeoPixel/NeoPixel.ino`, because the Arduino IDE insists on
  that, and the patches that drive it sit in the same folder. The same sketch
  may be copied into a second scenario; the check battery keeps the copies
  identical.
- Every patch has a one-line blurb in a `.txt` beside it, the sentence a
  reader sees first. Patches reference shipped files logically
  (`asset:...`), never start with `my-`, and open with no migration.
- Folder and file names are the menu's words: `Organisms/SerialOut/NeoPixel`
  is the path the menu shows, and `vu-meter.hum` reads as "Vu meter".
