# FilePlayer

Spielt Audiodateien ins Patch: eine Stereodatei oder bis zu 32 gleichzeitig, eine pro Ausgang.

Nimm ihn für Backing-Tracks, Field Recordings und einmalige Cues, die nicht dem Transport folgen müssen. Er läuft frei: Active startet und stoppt ihn, und mit dem Positionsbalken auf der Box springst du an jede Stelle, ob er gerade spielt oder pausiert. Jede Taste der Transportleiste ist ein Parameter, also bietet ein Rechtsklick auf jede davon MIDI-Learn, OSC-Learn, Steuern mit oder Folgen. Stopp im Transport lässt ihn weiterspielen; drück Stopp ein zweites Mal, wenn nichts mehr läuft, und er stoppt mit allem anderen. Loop wiederholt die Datei, auf Wunsch mit einer Pause zwischen den Durchgängen, und AutoRewind entscheidet, ob jeder Start dort weitermacht, wo er aufgehört hat, oder von vorn beginnt. Für Loops, die fest am Beat bleiben, nimm stattdessen LoopPlayer. Verbinde den Ausgang mit einem Mixer oder direkt mit SoundOut.

Outputs oben auf der Box macht ihn zu einem Mehrdatei-Player mit 4, 6, 8, 16, 24 oder 32 Ausgängen und einer Dateizeile pro Ausgang. Eine Datei in Zeile 1 spielt ab Ausgang 1, ein Ausgang pro Kanal: Eine Monodatei belegt einen Ausgang, eine Stereodatei in Zeile 1 die Ausgänge 1 und 2 und eine Sechskanaldatei sechs, während die Zeilen, die sie überdeckt, stumm bleiben. Alle Dateien starten, stoppen, loopen und springen gemeinsam, die längste bestimmt die Länge, und kürzere verstummen an ihrem Ende bis zum nächsten Durchgang. Wenn du Outputs änderst, bleiben die Dateien und alle Kabel erhalten, die noch passen; bei 2 ist er wieder der Stereo-Player.

## Parameter

**File** Die Audiodatei, die gespielt wird. Wähl sie mit der Datei-Schaltfläche oder zieh eine Datei auf den Slot.

**File_1-File_32** Die Datei jeder Zeile eines Mehrdatei-Players, sichtbar, wenn Outputs größer als 2 ist.

**Loop** Spielt die Datei jedes Mal wieder von vorn, wenn sie das Ende erreicht. Aus spielt sie einmal und verstummt dann.

**LoopDelay** Sekunden Stille zwischen dem Ende eines Durchgangs und dem Start des nächsten. Wirkt nur, solange Loop an ist.

**AutoRewind** Springt jedes Mal an den Anfang zurück, wenn Active angeht. Aus macht dort weiter, wo die Wiedergabe zuletzt gestoppt hat.

**Active** Startet und stoppt die Wiedergabe. Automatisiere es, um eine Datei an einer festen Stelle im Patch einsetzen zu lassen.

## Rezept

**Backing-Track auf Abruf** Lade die Datei, AutoRewind an, Loop aus. Verbinde den Ausgang mit einem Gain und dann mit SoundOut, und weise Active einem Button zu, sodass ein Druck den Titel jedes Mal von vorn startet; ein zweiter Druck stoppt ihn, und der nächste startet ihn wieder sauber.

## Verwandte Organisms

SoundIn, LoopPlayer, Drums
