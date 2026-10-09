# Crossfader

Ein Stereo-Crossfade zwischen zwei Quellen mit einem einzigen Bedienelement.

Die Eingänge 1-2 sind Seite A, die Eingänge 3-4 Seite B. Fade ganz unten lässt A durch, ganz oben B, und überall dazwischen mischt es beide nach der Blendkurve, die Curve wählt. Verkable zwei Deck-Organisms, zwei Loops oder eine unbearbeitete und eine bearbeitete Kopie desselben Signals hinein und führe den Ausgang weiter zu einem Mixer oder SoundOut. Gleiche die beiden Seiten mit TrimA und TrimB an, bevor du Curve anfasst: Ruckelt ein Fade, liegt das meist an zwei Quellen mit unterschiedlichem Pegel, nicht an der falschen Kurve. Fade kann über die Parametersteuerung auch andere Parameter steuern, sodass ein einziger Schwenk beim Überblenden einen Filter öffnet.

## Parameter

**MasterGain** Ausgangspegel nach dem Fade und den Trims.

**TrimA** Pegel von Seite A vor dem Fade, damit du die beiden Quellen zuerst angleichen kannst.

**TrimB** Dasselbe für Seite B.

**Fade** Die Crossfade-Position. Unten ist A, oben ist B.

**Curve** Die Blendkurve. Null ist eine gerade Linie, richtig für eine unbearbeitete und eine bearbeitete Kopie desselben Klangs; etwa zwei Drittel ist konstante Leistung, richtig für zwei voneinander unabhängige Quellen; eins ist ein schneller Cut, bei dem schon eine kleine Bewegung nahe einem der beiden Enden den Klang komplett umschaltet.

**CutA** Solange du ihn hältst, springt der Mix auf Seite A, egal was Fade sagt, und kehrt beim Loslassen zu Fade zurück. Hältst du A und B zusammen, parkt der Fade in der Mitte.

**CutB** Dasselbe für Seite B.

**CutTime** Wie lange ein Cut braucht, bis er greift, in Millisekunden. Null schaltet hart um; ein paar Millisekunden nehmen einem gehaltenen Bass den Klick.

## Rezept

**Zwei Decks überblenden** Verkable ein Deck mit den Eingängen 1-2 und ein anderes mit 3-4, stelle Curve auf etwa 0.67 und passe TrimA und TrimB an, bis jedes Deck mit Fade am jeweiligen Ende auf der Pegelanzeige des Mixers gleich laut ist. Weise CutA und CutB zwei Pads zu und stelle CutTime auf etwa 5, um den neuen Titel ohne Klicks über den laufenden zu cutten.

## Verwandte Organisms

Mixer, Gain, Console, Send
