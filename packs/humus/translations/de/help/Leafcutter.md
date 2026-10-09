# Leafcutter

Ein Loop-Chopper, der eine Audiodatei an ihren Transienten zerschneidet und die Slices im Takt des Transports in ihrer ursprünglichen Tonhöhe spielt.

Lade einen Loop, und er wird an seinen Transienten zerschnitten; die Slices folgen dann ohne Time-Stretching dem Tempo des Transports, sodass ein Break bei jedem Tempo sauber bleibt. Schnellere Tempi kürzen jeden Slice, langsamere lassen Luft zwischen den Schnitten. Geslicte Loop-Dateien, .rx2 und das ältere .rex, werden mit ihrer eigenen Slice-Map, ihrem Tempo und ihrer Schlagzahl geladen. Der MIDI-Eingang spielt Slices wie ein Kit, C3 für den ersten Slice und jede Taste darüber für den nächsten, sodass eine PianoRoll oder ein Keyboard den Loop spielen kann, mit oder ohne den selbstlaufenden Loop darunter. Verbinde den Ausgang mit einem Filter oder einem Fern.

## Die Map

Die Map zeigt den Loop mit einer Markierung an jedem Schnitt und hebt den Slice hervor, der gerade spielt. Zieh auf einem Slice nach oben oder unten, um ihn einzeln umzustimmen, und klick mit rechts darauf, um ihn umzukehren, stummzuschalten oder zurückzusetzen, oder um alle Slice-Änderungen zu verwerfen. Diese Änderungen liegen in SliceEdits und bleiben beim Patch.

## Parameter

**File** Der Loop zum Zerschneiden. Jede Audiodatei oder eine geslicte Loop-Datei mit eigener Slice-Map.

**Play** Der selbstlaufende Loop. Aus lässt nur das MIDI-Kit übrig.

**Sense** Wie viele der erkannten Transienten zu Schnitten werden. Die Erkennung passiert einmal beim Laden, daher schneidet der Regler sofort neu.

**Gate** Wie viel von jedem Slice-Fenster klingt, von abgehackt bis voll.

**Pitch** Transponiert jeden Slice in Halbtönen, -24 bis 24, per Resampling.

**Reverse** Spielt jeden Slice rückwärts.

**Decay** Blendet jeden Slice ab seinem Start aus. 0 lässt die Slices ganz; ganz oben ist jeder Slice ein kurzer Schlag.

**Shuffle** Seed für die Slice-Reihenfolge. 0 spielt den Loop wie geschrieben; jede andere Zahl ist eine wiederholbare Umstellung.

**Beats** Loop-Länge in Schlägen. 0 nimmt sie aus der Datei oder schätzt sie aus Länge und Tempo.

**Level** Ausgangspegel.

**SliceEdits** Die Tonhöhen-, Umkehr- und Stumm-Änderungen pro Slice, die auf der Map gemacht wurden, gespeichert mit dem Patch.

## Rezept

**Neu zerhackter Break** Lade einen zweitaktigen Drumloop, Beats 8, Sense 0.6, Gate 0.7, Shuffle 17, Decay 0.2. Starte die Wiedergabe, und der Break spielt umgestellt im Takt; drück Zufall für eine andere Reihenfolge und verbinde den Ausgang mit einem Filter mit LfoSync an.

## Verwandte Organisms

Sampler, Deck, PianoRoll, Repeater
