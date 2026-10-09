# Sampler

Ein polyphoner Sample-Player mit Zonen-Mapping, acht Slots, Laden von Soundbanks und Aufnahme direkt aus dem Patch.

Lade bis zu acht Audiodateien, gib jeder einen Grundton und spiel sie per MIDI von einem MidiIn, einer PianoRoll oder der Bildschirmtastatur. Oder lade eine ganze Soundbank-Datei (.sf2 oder die Sample-Bibliotheken .nsmp3 und .nsmp4 eines Stage-Keyboards) und spiel ihre Instrumente; eine geladene Bank ist das Instrument, und die Slots spielen nur, wenn keine Bank geladen ist. Verbinde ein Signal mit dem Audio-Eingang, und Record nimmt einen Take direkt in einen Slot auf, als Audiodatei, die mit dem Patch gespeichert wird. Er startet grob, zwölf Bit bei 32 kHz, in der Tradition der Rack-Sampler der späten Achtziger; die Preset-Leiste reicht von Ancient bis Pristine und verändert dabei nur Bits und Rate. Velocity skaliert den Pegel, und jenseits von Polyphony übernimmt eine neue Note die älteste Stimme.

## Slots und Bänke

Im Modus Pitched teilen die Zonen die Tastatur an den Mittelpunkten zwischen ihren Grundtönen, und Noten werden per Resampling vom nächstgelegenen Grundton aus gestimmt; eine einzelne Zone überspannt die ganze Tastatur. Im Modus Kit spielt jede Zone nur auf ihrer Grundton-Taste in Originalgeschwindigkeit, für One-Shots auf einzelnen Tasten. Eine Bank bringt ihre eigenen Tastaturaufteilungen, Loop-Punkte und Hüllkurven mit, und die Hüllkurvenregler skalieren diese dann, statt sie zu ersetzen. Takes landen in einem Samples-Ordner unter Dokumente.

## Parameter

**Mode** Pitched verteilt die Zonen nach Grundton über die Tastatur. Kit spielt jede Zone nur auf ihrer Grundton-Taste.

**Gain** Ausgangspegel.

**BendRange** Wie weit das Pitch-Bend-Rad bei vollem Ausschlag reicht, in Halbtönen. Zwei ist der übliche Standard; null ignoriert das Rad.

**Attack** Anstiegszeit jeder Note in Millisekunden. Mit geladener Bank skaliert es das Attack der Zone, sodass der Standardwert die Bank so spielt, wie sie geschrieben ist.

**Decay** Abfallzeit bis zum Sustain-Pegel in Millisekunden, mit einer Bank genauso skaliert.

**Sustain** Der gehaltene Pegel, 0 bis 1.

**Release** Ausklingzeit nach dem Loslassen der Taste, in Millisekunden.

**Polyphony** Die Anzahl gleichzeitig klingender Stimmen. Darüber hinaus übernimmt eine neue Note die älteste Stimme.

**Loop** An hält eine Note auf den Loop-Punkten, die die Datei mitbringt, solange du die Taste gedrückt hältst; eine Datei ohne Loop-Punkte spielt so oder so durch. Aus ignoriert alle Loop-Punkte und lässt jede Note bis zum Ende ihres Samples laufen; das ist die richtige Wahl, wenn die Loops einer Bibliothek schlecht sitzen. Der Übergang wird überblendet, sodass ein Loop, dessen Enden nicht zusammenpassen, sauber hält, statt einmal pro Durchlauf zu klicken.

**File1** Die Audiodatei in Slot 1, ebenso für File2 bis File8.

**Root1** Die MIDI-Note, auf der Slot 1 in Originaltonhöhe spielt, standardmäßig 60, ebenso für Root2 bis Root8. Eine Datei, die ihre eigene Taste mitspeichert, setzt sie beim Laden; für ein Kit stell jeden Grundton auf die Taste, auf der du den Klang haben willst.

**FileBank** Eine Soundbank-Datei. Solange eine geladen ist, spielt sie.

**Preset** Welches Instrument der Bank antwortet, nach Namen aufgelistet.

**RecSlot** Der Slot, in dem ein Take landet.

**RecordDuration** Beendet einen Take nach so vielen Millisekunden von selbst. Bei 0 läuft ein Take, bis du ihn stoppst oder die Grenze von dreißig Sekunden erreicht ist.

**Record** Startet einen Take vom Audio-Eingang. Drück noch einmal, um zu stoppen.

**Bits** Wortbreite der Samples, 8 bis 24. Unter 24 wird die Datei grob gespeichert, wie es frühe Hardware-Sampler taten, mit Dither, damit die Grobheit als gleichmäßiges Grundrauschen ankommt statt als Brummen, das der Note nach unten folgt. Eine Änderung liest die Datei neu ein. Die Pfeile springen durch die üblichen Tiefen 8, 12, 16 und 24; jede andere Zahl tippst du ins Feld.

**Rate** Samplerate in kHz, 4 bis 48. Die Datei wird auf die neue Rate heruntergefiltert, bevor sie ausgedünnt wird, so wie es der Wandler vor einem Rack-Sampler tat; was über der neuen Obergrenze lag, wird also leise, statt als metallisches Klingeln zurückzuspiegeln. Bei 48 bleibt die Datei unangetastet. Die Pfeile springen durch die üblichen Raten 4, 8, 11, 16, 22, 32, 44 und 48; jede andere Zahl tippst du ins Feld.

**Monitor** Leitet den Eingang an die Ausgänge weiter, damit du hörst, was du gleich aufnimmst. Standardmäßig aus, damit ein Sampler, der schon an einem Mixer hängt, das Signal nicht verdoppelt.

**Threshold** Hält einen aufnahmebereiten Take zurück, bis der Eingang diesen Pegel erreicht, gezeichnet als Markierung über der Pegelanzeige. Ganz links nimmt er sofort auf, sobald du scharf schaltest.

## Rezept

**Kit aus dem Patch** Mode Kit, Root1 36, Root2 38, Root3 42. Verbinde einen Kick mit dem Audio-Eingang, RecSlot 1, Threshold 0.05, Record an, und lös den Kick einmal aus; wiederhol das für die anderen Slots mit anderen Quellen. Verbinde ein Riff oder eine PianoRoll mit dem MIDI-Eingang und spiel das Kit.

## Verwandte Organisms

PianoRoll, MidiIn
