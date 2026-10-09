# Siren

Eine Soundsystem-Sirene: ein rauer Oszillator, den eine langsame Rampe zwischen zwei Tonhöhen hin und her fährt, gehalten, so lange du willst.

Unser eigenes Design nach dem Vorbild der klassischen Zwei-Transistor-Kisten, mit denselben vier Reglern auf der Frontplatte, den beiden Enden der Tonhöhe und den beiden Hälften des Sweeps, und mit den Schaltern, die diese Kisten an der Seite haben. Halte den Hold-Knopf, halte ein ihm zugewiesenes Pad oder halte eine beliebige Note auf einem MIDI-Kabel; jeder Druck startet den Sweep von vorn, und beim Loslassen klingt sie über die Release-Zeit aus. Der Ausgang ist mono. Die Pegelanzeige folgt dem Sweep, und Sweep und Gate sind auch Steuerwerte, sodass ein Siren ein Licht oder ein Filter steuern kann, während es heult.

## Parameter

**PitchLow** Das untere Ende des Sweeps in Hz. Liegt es über PitchHigh, läuft der Sweep andersherum.

**PitchHigh** Das obere Ende des Sweeps in Hz.

**LfoUp** Wie lange die Rampe zum Steigen braucht, in Sekunden. Ungleiche Up- und Down-Zeiten ergeben das schiefe Heulen.

**LfoDown** Wie lange die Rampe zum Fallen braucht, in Sekunden.

**Mode** Sweep steigt und fällt. Rise steigt und springt zurück, dann ruht es für die Down-Zeit. Fall fällt und springt hoch und ruht für die Up-Zeit. Step springt zwischen den beiden Tonhöhen hin und her, der Zweiklang.

**Wave** Square, Pulse oder Saw für den Oszillator. Square ist der Klang der Kiste.

**Tone** Ein Tiefpass am Ausgang. Dreh ihn zurück für ein dumpferes Horn.

**Auto** An: Die Rampe läuft von selbst. Aus: Manual stellt die Tonhöhe von Hand ein.

**Manual** Wo zwischen PitchLow und PitchHigh die Tonhöhe liegt, solange Auto aus ist, sodass ein zugewiesener Regler von Hand heult.

**Hold** Lässt die Sirene ertönen, solange er gedrückt ist.

**Release** Wie schnell der Klang verklingt, sobald du loslässt, in Millisekunden.

**Level** Ausgangs-Gain.

## Rezept

**Zweiton-Horn** Mode Step, Wave Square, PitchLow 400, PitchHigh 530, LfoUp 0.4, LfoDown 0.4, Tone 3000. Weise Hold einem Pad zu, führe den Ausgang durch ein Fern mit einer kurzen, synchronisierten Wiederholung und drück zwei Takte lang zu Beginn eines Drops.

## Verwandte Organisms

Kick, LFO, Button
