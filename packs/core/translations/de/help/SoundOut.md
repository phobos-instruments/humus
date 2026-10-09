# SoundOut

Der Audioausgang: Alles, was er empfängt, geht ans Interface.

Schließ den Mixer oder ein anderes letztes Organism daran an und wähl das Ausgangspaar; das Dropdown Output listet die eigenen Ausgangsnamen des Interfaces auf, und die Pegelanzeige zeigt, was hinausgeht, nach Gain. Das erste SoundOut in einem Patch ist der Master, den die Pegelanzeige in der Werkzeugleiste zeigt, und ein neues Patch beginnt mit einem. Setz weitere für Monitor-Sends, Kopfhörer-Cues oder Surround-Stems ein, jedes auf sein eigenes Paar gerichtet; wo zwei denselben Kanal ansprechen, addieren sich ihre Signale. Ein SoundOut übernimmt seine Kanalzahl von dem, was an ihm angeschlossen ist, mindestens stereo; ein Sechskanal-Signal belegt also sechs aufeinanderfolgende Geräteausgänge ab dem gewählten Kanal. Ältere Patches mit nummerierten Zusatzausgängen lassen sich weiterhin laden, jeder mit seinem Kanal als Parameter.

## Parameter

**Channel** Welches Ausgangspaar des Geräts dieses SoundOut speist, angezeigt als Output; 1/2 ist das Hauptpaar. Eine Änderung greift sofort.

**Gain** Eine saubere Ausgangsanpassung, standardmäßig auf Unity. Die Pegelanzeige misst danach, zeigt also genau, was zum Interface hinausgeht.

**DcGuard** Ein sanfter 5-Hz-Hochpass am Ausgang, standardmäßig an, damit ein konstanter Wert aus einem Number oder einem Follower zu einem kurzen Klick und dann zu Stille wird, statt eine Lautsprechermembran festzuhalten. Schalte es nur an einem Ausgang aus, der ein DC-gekoppeltes Interface für Steuerspannung speist.

## Rezept

**Kopfhörer-Cue** Lass das erste SoundOut für den Raum auf 1/2 und stell ein zweites auf 3/4, mit den Kopfhörern an den Ausgängen 3 und 4 des Interfaces. Schließ das Send-Paar eines Sends oder einen zweiten Mixer daran an und stell seinen Gain separat ein, damit der Cue-Pegel nie den Hauptmix berührt.

## Verwandte Organisms

SoundIn, Mixer, FileRecorder
