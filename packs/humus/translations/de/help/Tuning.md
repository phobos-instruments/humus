# Tuning

Die Stimmung des Patches: Jedes Organism, das eine Note in eine Frequenz umwandelt, fragt hier nach der Antwort.

Es macht keinen Klang und sendet keine Noten. Gleichstufige Stimmungen, reine Stimmung aus exakten Verhältnissen, die Obertonreihe und jede .scl-Tonleiterdatei stehen alle in einem Tonleiter-Browser, und Map entscheidet, wie gewöhnliche Notennummern ihre Tonhöhe in einer Tonleiter ohne zwölf Stufen finden. Die eingebauten Instrumente, Rhizome, Substrate, Wave, pH, Mineral, Acid, Microdot und Trellis, folgen ihm direkt; gehostete Plug-ins erfahren es über MIDI, wie die Zeile Plugins beschreibt. Die Notenrolle zeichnet weiterhin zwölf Zeilen pro Oktave, also passen unter Map Every step in einer Tonleiter, die nicht 12 Stufen hat, ihre Beschriftungen nicht mehr zu den Tonhöhen, auch wenn die Noten stimmen.

## Organisms erreichen

Verkable seinen MIDI-Ausgang mit einem Instrument, und dieses Instrument spielt in dieser Stimmung. Die Stimmung fließt die MIDI-Kabel hinunter weiter, sodass Tuning in DNA in Rhizome beide stimmt, und zwei Tuning-Organisms setzen zwei Synths gleichzeitig in zwei Stimmungen. Lässt du es unverbunden, stimmt es das ganze Patch; das ist der normale Fall und der einzige Weg, ein Organism ohne MIDI-Eingang zu erreichen, etwa Trellis.

## Parameter

**Preset** Der Modus, unten in der Box. Scale file, die Voreinstellung, spielt die Tonleiter, die oben im Browser gewählt ist, und ein neues Tuning startet dort auf 12-TET, der gewöhnlichen Stimmung, die eine Tonleiterdatei wie jede andere ist. Custom EDO teilt die Oktave in eine eigene Anzahl gleicher Divisions und graut den Browser aus, der seine Tonleiter behält, bis du zurückkommst. Die Tonleitern, die diese Liste früher enthielt - 12-TET, 19-, 24- und 31-EDO, Bohlen-Pierce, die reinen 5-, 7- und 17-Limit-Tonleitern, Harmonics 8-16 und Pythagorean - stehen jetzt im Browser, Note für Note gleich. Ein Patch, das mit einer davon gespeichert wurde, öffnet sich weiterhin damit, zeigt hier ihren Namen und spielt wie immer.

**Divisions** Gleiche Stufen pro Oktave für Custom EDO, 1 bis 128. Eins sind nur Oktaven, zwei fügt den Tritonus hinzu, und 128 ist das Maximum, das eine Tonleiter fassen kann. Im Modus Scale file ausgegraut, denn dort entscheidet die Datei.

**Root** Die MIDI-Note, die an RootHz festgemacht ist. 69 ist A4.

**RootHz** Die Frequenz, auf der diese Note klingt. 440 ist Standard.

**Map** Wie eine Taste ihre Note findet, wenn die Tonleiter keine zwölf davon hat. Every step spielt auf jeder Taste die nächste Note der Tonleiter, sodass jede Note erreichbar ist, eine 19-tönige Oktave 19 Tasten braucht und ein Wechsel der Tonleiter alles verschiebt. Fit to 12 keys verteilt eine Periode der Tonleiter über eine gewöhnliche Oktave mit zwölf Tasten und spielt die Note, die dem üblichen Platz jeder Taste am nächsten liegt, sodass eine siebentönige reine Tonleiter auf den weißen Tasten landet und die Oktavtaste eine echte Oktave bleibt; bei mehr als zwölf Noten bekommen manche keine Taste. Bei einer zwölftönigen Tonleiter sind beide gleich.

**File** Die Tonleiter, oben in der Box: die .scl-Datei, die der Modus Scale file spielt. Klick auf den Namen, um den Tonleiter-Browser zu öffnen: die Tonleitern, die mit der App kommen, deine eigenen, die, die du von anderswo mitgebracht hast, und die, die du zuletzt verwendet hast, jede mit ihrer Notenzahl und Beschreibung, und eine Suche, die all das liest. Die Pfeile springen zur nächsten und vorigen Tonleiter. Eine Tonleiter zu wählen wählt auch das Preset aus. Eine fehlende Datei wird bernsteinfarben markiert und spielt als einfaches 12-TET.

**Plugins** Wie gehostete Plug-ins es erfahren. MTS SysEx sendet Stimmungsnachrichten für einzelne Noten an Plug-ins, die den Standard verstehen. Pitch bend legt jede Note auf ihren eigenen Kanal, mit einem Bend für den Rest, was fast überall funktioniert, aber die Polyphonie begrenzt und das Bend-Rad übernimmt. Auto (probe) testet jedes Plug-in einmal und merkt sich das Ergebnis.

## Rezept

**Schwebungsfreie Quinte** Preset Just - 17-limit, Map Fit to 12 keys, lass es unverbunden, damit das ganze Patch folgt. Halte über eine PianoRoll eine Quinte auf einem Rhizome: Sie rastet ohne Schwebung ein, wie es keine temperierte Quinte kann. Dann wechsle zu Harmonics 8-16 und spiel eine Tonleiter auf demselben Rhizome, für die Obertonreihe selbst.

## Verwandte Organisms

Rhizome, Trellis, Substrate
