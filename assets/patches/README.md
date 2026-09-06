# Patches

The patches the app ships. They are offered in three places: the welcome tour's
**Load a Demo Patch...**, the **Start here** column of the start window, and
anywhere else that asks for `assetSearchPath("patches")`.

Drop a `.hum` / `.amh` file in here and it is picked up automatically: the
build stages the whole `assets/` tree rather than naming files, so a new patch
needs no CMake edit. Three rules:

1. **Reference assets logically.** A shipped sample, scale or shader is named
   `asset:Samples/Analog/kick.wav`, never by absolute path, or the patch only
   opens on the machine that saved it. `asset-ref-check` fails the build's
   battery when one does not resolve.
2. **A name starting with `my-` is ignored.** That is the escape hatch for the
   working files a developer leaves in this folder; they are neither offered
   nor staged.
3. **Give it a one-line blurb.** A `<name>.txt` beside the patch, whose first
   line is a short lower-case phrase, is what the start window prints under the
   patch's name. Without one it says "a patch that ships with Humus".

Tutorial patches belong here too: one idea each, named for what they teach, and
the blurb is the sentence the reader sees before opening it.
