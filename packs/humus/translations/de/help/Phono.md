# Phono

Ein Phono-Vorverstärker: RIAA-Entzerrung, Tonabnehmer-Gain und ein Rumpelfilter für einen Plattenspieler an einem Line-Eingang.

Platten werden mit abgesenkten Bässen und angehobenen Höhen geschnitten, und ein Tonabnehmer liefert nur Millivolt, sodass eine Platte, die direkt in einen Line-Eingang gespielt wird, dünn und sehr leise klingt. Phono macht die Kurve rückgängig und stellt den Pegel wieder her. Der Entzerrungsfilter ist an die analoge RIAA-Kennlinie bei der laufenden Samplerate angepasst, die oberste Oktave eingeschlossen. Setz es direkt hinter ein SoundIn und vor ein Deck, eine Console oder einen Mixer. Wenn der Kanal deines Interfaces einen Phono/Line-Schalter hat, stell ihn auf Line, sonst wird die Korrektur doppelt angewendet. Der Signalfluss ist Entzerrung, dann Rumpelfilter, dann Gain.

## Parameter

**Curve** RIAA ist die Standard-Wiedergabekurve. RIAA + IEC ergänzt einen sanften Roll-off bei 20 Hz, den manche Phono-Vorstufen haben. Flat überspringt die Entzerrung und behält nur den Gain, für Überspielungen, die du später entzerrst, oder für Platten, die älter als der Standard sind.

**Gain** Tonabnehmer-Gain in dB. Ein Moving-Magnet-Tonabnehmer, und das ist jeder übliche DJ-Tonabnehmer, will etwa 40; ein Moving-Coil-Tonabnehmer mit niedriger Ausgangsspannung will etwa 60. Zu hoch übersteuert.

**Rumble** Ein Hochpass bei 18 Hz, der das Wabern verwellter Platten und das Rumpeln des Plattenspielers entfernt, bevor sie Headroom fressen. Standardmäßig an; schalte ihn für Messungen aus.

## Rezept

**Plattenspieler anschließen** Verbinde SoundIn mit Phono und Phono mit einem Deck oder einem Mixer. Curve RIAA, Rumble an, Gain 40 für einen Moving-Magnet-Tonabnehmer; erhöhe Gain in Fünferschritten, bis eine laute Passage auf den Pegelanzeigen knapp unter Vollaussteuerung liegt.

## Verwandte Organisms

Console, Gain, Deck, SoundIn
