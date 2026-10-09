# VideoPlayer

Ein Videodeck, das ein Band als Loop in die Videokette spielt.

Leg ein Band in den File-Slot, oder zieh einen Clip von der Timeline darauf, und verbinde den Video-Ausgang mit einer Lumen-Ebene, einem VideoFX, einem VideoMix oder einem VideoOut; dort läuft das Band als Loop. Rate ist der Motor, Opacity und Blend bestimmen, wie die Ebene sich einfügt, alles gewöhnliche Parameter, sodass ein Follower die Geschwindigkeit mit der Musik steuern kann. Der MIDI-Eingang ist ein Neustart-Trigger: Jedes Note-on spult das Band zurück und startet es neu, deshalb schneidet ein Deck pro Startspur, gespeist von einem Sequence oder einer PianoRoll, Video im Takt. Das Deck läuft, solange sein Editor offen ist oder etwas weiter hinten in der Kette seinen Ausgang beobachtet, und ruht sonst.

## Parameter

**File** Das Video, das gespielt wird.

**Follow** Übernimmt die Bildposition aus Position, statt mit eigener Rate zu spielen. Verbinde den Positions-Ausgang eines Decks mit Position, und das Video folgt der Platte: Scratche sie, schubs sie an, spul sie zurück, und das Bild geht mit der Hand mit. Aus: Die Datei spielt von selbst mit Rate.

**Position** Welche Stelle des Videos gezeigt wird, von seinem Anfang bis zu seinem Ende, wenn Follow an ist.

**Rate** Der Speed-Regler, in Prozent. 100% ist wie aufgenommen, 0% hält ein Standbild, 200% verdoppelt das Tempo.

**Opacity** Wie stark diese Ebene in einem Lumen über den darunterliegenden zu sehen ist.

**Blend** Wie sie sich mit diesen Ebenen verbindet: Normal, Add, Multiply oder Screen.

## Rezept

**Notengeschnittener Loop** Lade ein kurzes Band, verbinde den Ausgang mit einem VideoOut und ein Sequence mit dem MIDI-Eingang, mit einer Note pro Takt, sodass das Band auf der Eins neu startet. Leg einen langsamen, flachen LFO um 100% auf Rate, und der Loop atmet gegen das Raster.

## Verwandte Organisms

Lumen, VideoMix, VideoOut
