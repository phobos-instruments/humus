# Body model

`body.humnet` is MediaPipe's pose detector and pose-landmark model (full),
converted by `tools/pose_model_convert.py` from Google's
`pose_landmarker_full.task` bundle and run by `packs/humus/common/HumNet.h` -
the same small interpreter that runs the hand model, taught one more operator
for this graph. None of MediaPipe's own code is vendored, only the trained
weights.

**Licence:** Apache-2.0, stated on the MediaPipe Pose model card
("MediaPipe BlazePose GHUM 3D", 2021). The model card also records the
limits: trained for a single nearby person, meant for fitness-style framing,
and unreliable with heavy occlusion or several people in the frame.

The file is gzipped float16 weights plus a small graph table - 8.1 MB on
disk. The full landmark variant is carried rather than the lite one: on a
dancer in loose sleeves the lite model lost both arms to the backdrop while
full tracked them, and the arms are most of what the Skeleton organism
reads; the price is roughly half the tracking rate, still comfortably
inside the smoothing. The converter densifies the detector's sparse weights and prunes the
landmark model's segmentation-mask, heatmap and world-coordinate heads, which
Body never reads, so the file carries about a third less than the bundle.
**This** repository carries it. The **source distribution** does not:
`.gitattributes` drops it, and `engine/CMakeLists.txt` rebuilds it at
configure time from Google's own bundle - download, unzip, convert, check the
digest - so nothing is served from us and the model comes from where it came
from in the first place. A build that cannot reach Google, or has no Python,
warns and carries on; skeleton tracking then does nothing on that build. Bytes
that are not the model stop the configure instead.

It is regenerated, not edited. What the build does, you can do:

    base=https://storage.googleapis.com/mediapipe-models/pose_landmarker
    curl -LO $base/pose_landmarker_full/float16/1/pose_landmarker_full.task
    unzip pose_landmarker_full.task
    cd tools && python3 pose_model_convert.py \
        ../pose_detector.tflite ../pose_landmarks_detector.tflite \
        ../packs/av/assets/Models/body/body.humnet

The bundle is sha256 `5134a3aad27a58b93da0088d431f366da362b44e3ccfbe3462b3827a839011b1`
and the model it converts to is
`a4039232262fc38c7220b58372792ab7358856441191386ecb1988519d9495cb`; both are
pinned in `engine/cmake/PoseModel.cmake`. The conversion is reproducible -
gzip's header timestamp is written as zero for exactly that reason - so those
two digests are the check that a rebuild is the same model.

The Skeleton organism runs it on every platform; there is no system-tracker path.
The conversion itself is pinned by the skeleton test suite, which runs both nets
on a synthetic frame and expects the values the reference TFLite runtime
produced from the original bundle.

## The Neural Engine packages

`det.mlpackage` and `lm.mlpackage` are derived from the .humnet by
`tools/humnet_to_coreml.py` and are NOT committed - CMake converts them at
configure time into `engine/build/mlmodels/` and the staging steps merge them
into the shipped assets. The converter needs a Python with coremltools:

    python3.12 -m venv .venv-coreml
    .venv-coreml/bin/pip install coremltools numpy

then re-run cmake. Without it the build still works and the tracker runs on
the portable interpreter; a release build should have it so players get the
Neural Engine speed.
