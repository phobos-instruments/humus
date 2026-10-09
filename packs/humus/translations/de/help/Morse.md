# Morse

Ein Pattern-Generator, der eine getippte Nachricht als Sinuston und als MIDI-Noten in echtem Morse-Timing tastet.

Tipp in Text, und die Nachricht wird mit Dits, Dahs, Buchstaben- und Wortpausen getastet, fest am Transport: still, solange er steht, und ab dem Anfang der Nachricht, sobald die Wiedergabe startet. Der Ton folgt der Stimmung des Patches, und die Tastflanken sind 3 ms weich, damit nichts klickt. Der MIDI-Ausgang tastet dieselbe Nachricht als Paare aus Note-on und Note-off auf der Tonhöhe von Note: Schick ihn in einen Kick, und die Nachricht wird zum Rhythmus, oder in ein Acid, und sie spielt als Linie; mit Level auf 0 ist es eine reine MIDI-Pattern-Quelle. Schick den Ton für eine Melodielinie durch ein Fern.

## Parameter

**Text** Die Nachricht. Buchstaben, Ziffern und gängige Satzzeichen; was sich nicht morsen lässt, wird übersprungen. Das Band darunter zeichnet die Nachricht als Dits und Dahs und lässt das Zeichen aufleuchten, das gerade getastet wird.

**WPM** Tastgeschwindigkeit in Wörtern pro Minute, 5 bis 40. Eine Einheit dauert 1.2 geteilt durch WPM Sekunden.

**Sync** Ignoriert WPM und macht eine Einheit genau eine Sechzehntel lang, sodass die Nachricht zum Rhythmus auf dem Raster wird und Tempowechsel sie neu takten. Die Nachricht beginnt auf Schlag 1, und ihre Position folgt dem Transport, sodass ein Sprung oder Loop auf der Timeline jedes Mal auf demselben Buchstaben landet.

**Note** Die Tonhöhe des Tons als Note, nach der Stimmung des Patches.

**Loop** Wiederholt die Nachricht mit der üblichen Wortpause von sieben Einheiten zwischen den Durchgängen. Aus spielt einen Durchgang pro Wiedergabe.

**Level** Ausgangspegel des Tons.

## Rezept

**Telegrafen-Rhythmus** Text CQ CQ, Sync an, Loop an, Note D5. Schick den MIDI-Ausgang in einen Kick und den Audioausgang durch ein Fern mit Sync an auf 1/8; starte die Wiedergabe bei 140 BPM, und die Nachricht tastet den Kick.

## Verwandte Organisms

Rhizome, Sequence, SideKick
