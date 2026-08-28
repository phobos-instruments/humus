# Hand models

`hands.humnet` is MediaPipe's palm detector and hand-landmark model, converted
by `tools/hand_model_convert.py` from Google's `hand_landmarker.task` bundle and
run by `packs/humus/organisms/Hands/HandNet.h` - a small interpreter over the
twelve operators the two graphs actually use. None of MediaPipe's own code is
vendored, only the trained weights.

**Licence:** Apache-2.0, stated on the MediaPipe Hands model card
("MediaPipe Hands (Lite/Full)", October 2021). The model card also records the
limits: trained on limited data, meant for experimental use, and unreliable
with gloves, heavy occlusion or hands holding objects.

The file is gzipped float16 weights plus a small graph table - 5.5 MB on disk,
7.7 MB in memory. **This** repository carries it. The **source distribution**
does not: `.gitattributes` drops it, and `engine/CMakeLists.txt` rebuilds it at
configure time from Google's own bundle - download, unzip, convert, check the
digest - so nothing is served from us and the model comes from where it came
from in the first place. A build that cannot reach Google, or has no Python,
warns and carries on; hand tracking then does nothing where the system has no
tracker of its own. Bytes that are not the model stop the configure instead.

It is regenerated, not edited. What the build does, you can do:

    base=https://storage.googleapis.com/mediapipe-models/hand_landmarker
    curl -LO $base/hand_landmarker/float16/1/hand_landmarker.task
    unzip hand_landmarker.task
    python3 tools/hand_model_convert.py \
        hand_detector.tflite hand_landmarks_detector.tflite \
        assets/Models/hands/hands.humnet

The bundle is sha256 `fbc2a30080c3c557093b5ddfc334698132eb341044ccee322ccf8bcf3607cde1`
and the model it converts to is
`e531b99d34cf03a7d668f03c35e9fff48137ae471604f632f83f66e9ee71c522`; both are
pinned in `engine/CMakeLists.txt`. The conversion is reproducible - gzip's
header timestamp is written as zero for exactly that reason - so those two
digests are the check that a rebuild is the same model.

The Hands organism uses it where the system has no hand tracker of its own.
Where the system has one, the two are compared landmark by landmark, so a bad
conversion is caught rather than merely looking plausible.
