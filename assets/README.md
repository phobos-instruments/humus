# Assets

What Humus ships to be USED rather than compiled. One folder name, three
places, the same shape in each:

| | Holds | |
| --- | --- | --- |
| `assets/` here | patches, scales, shaders, models | what the build stages |
| inside the app | the same, minus your `my-*.hum` | read-only defaults |
| `Documents/Humus/assets/` | scales, shaders, banks | yours |

Both the app's set and yours are read, yours first, so a file of your own wins
by name and an update can improve the defaults without touching it. Nothing is
copied out of the app into your folder.

| | Your folder |
| --- | --- |
| macOS | `~/Documents/Humus/assets/` |
| Windows | `%USERPROFILE%\Documents\Humus\assets\` |
| Linux | `$XDG_DOCUMENTS_DIR/Humus/assets/`, or `~/Documents/Humus/assets/` |

Two things deliberately live outside this tree. **Icons** are branding, decided
before a build: they are drawn as vectors and compiled into the binary, so
there is no icon folder anywhere. **Voice and sound banks** ship with the
organism that plays them (`packs/humus/organisms/Ph/banks/`), because a
downloadable pack has to carry its own; only the user's own banks are here, in
`banks/pH/` and `banks/Sampler/`.

## Impulses/

Fourteen impulses in three groups - `Room/` (eight, tight to long),
`Cabinet/` (three) and `Spring/` (three). The folder name is the badge the
browser shows beside each one, and the reference carries it:
`asset:Impulses/Room/3.wav`.

From the **Adventure Kid Reverb Impulse Responses** by Kristoffer Ekstrand,
[adventurekid.se](https://www.adventurekid.se/akrt/free-reverb-impulse-responses/),
licensed **CC BY 4.0**. Changed from the originals: converted from 32-bit float
to 24-bit, and renamed for the browser. The originals are `AK-SROOMS_*`
(rooms), `AK-SPKRS_*` (cabinets), `AKIR_Springer_*` and `AKIR_DualSpringer_*`
(springs).

## Samples/

Ten one-shots in `Analog/`: kick, snare, clap, closed and open hat, rim,
bell, cymbal, and two toms. The **Analog Kit** preset wires eight of them
into the Sampler's slots.

`Banks/` is the other half of this pair - many instruments in one file, .sf2
for the Sampler and .syx or .wopn for pH. It holds one soundfont built from the
kit above; pH's own voice banks carry inside its pack.

Recorded from the individual outputs of a single machine by Michael Fischer,
published as `sounds-tr808-fischer` and dedicated to the public domain under
**CC0 1.0**. Unchanged apart from the filenames. CC0 asks for no attribution;
this note records provenance because a sound file cannot carry its own.

## patches/

The patches the app ships, offered by the welcome tour under **Load a Demo
Patch...**. Each one is described in `assets/patches/README.md`.

## Scales/

Scala `.scl` files, the format the Tuning organism reads, in five collections:
temperaments, just intonation, equal divisions, non-octave and world. Twelve-
tone equal temperament needs no file - it is what Tuning does with no scale
loaded.

They are written for this repository rather than taken from anyone's
collection, and every tempered value is computed from its definition (the chain
of fifths and which of them are narrowed) rather than copied from a table.
The maqam and shruti files are the written theory rather than a measurement,
and say so. The two gamelan scales are the opposite: one named instrument's own
tones, stretched octave included, cited in the file. A tuning is a fact and a
citation is not a licence, so nothing is copied from anyone's archive - the
numbers are re-set in our own files.

The Scala archive (~4,500 more) is a download away at
huygens-fokker.org/scala/downloads.html. It is other people's work under their
own terms, which is why it is not vendored here: unzip it into your Documents
folder above and every one of them shows up, grouped by its own folder names.
A dev tree gets no special treatment - what you see building from source is
what a user sees.

## Models/

Inference models, one folder each. `Models/hands/hands.humnet` is the hand
tracker used where the system has none of its own; its README carries the model
card and the commands that rebuild it. Nothing here is edited - a model is
regenerated from what it was converted from.

## Shaders/

GLSL scenes for Lumen. A patch names one as `asset:shaders/breathe.frag` - a
logical name, not a path - and the app finds it wherever assets are on that
machine. So a patch that uses a shipped shader opens on anyone's computer, and
moving this folder cannot break one. A name may be nested
(`asset:scales/Just Intonation/just-major.scl`).

Absolute paths and document-relative ones still work for anything already
written that way; the logical form is what the shipped patches use.

## What the source distribution leaves out

`Impulses/`, `Samples/`, `Banks/` and `Models/hands/hands.humnet` are empty in
the source zip - the folders and their READMEs ship, the content does not. The
first three are recordings made by other people, offered under licences that
ask for credit rather than for silence, and the released app is where that
credit belongs; the fourth is rebuilt at configure time from the bundle it was
converted from. `Scales/`, `Shaders/` and `patches/` are written for this
repository and ship in full - all three together are under 250 KB.
