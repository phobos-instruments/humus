# Models

Inference models, one folder each, named for what the model does rather than
for where it came from. `hands/` is the only one so far.

This is not a browsable kind. Every other folder under `assets/` corresponds to
something a user picks in a browser - impulses, samples, banks, scales, shaders
- and `asset-layout-check` holds that line by failing on any shipped folder
that is not one. `Models/` is exempt beside `patches/`: nothing here is chosen
by hand, it is loaded by the organism that needs it.

A model is **regenerated, not edited**. Each folder carries a README with the
licence, the model card and the exact commands that rebuild the file from
whatever it was converted from, and `engine/CMakeLists.txt` pins the digest of
both the input and the result. That is what makes a rebuild checkable rather
than merely plausible.

The weights do not travel with the source distribution - they are somebody
else's, and large. The zip ships this folder and the READMEs; the configure
step rebuilds the files, and says so loudly if it cannot.

Referenced as `asset:Models/<model>/<file>`, which resolves against the user's
assets folder first and the app's second, like every other reference.
