# FrequencyShifter

Addiert eine feste Anzahl Hertz zu jeder Frequenz im Signal.

Ein Pitch-Shifter multipliziert jede Frequenz mit demselben Verhältnis, sodass die Obertöne im Gleichschritt bleiben und der Klang seine Identität behält. Dieses Organism addiert stattdessen zu jeder dieselbe Anzahl Hertz: Eine Reihe bei 100, 200 und 300, um 50 verschoben, wird zu 150, 250 und 350, und die ist nicht mehr harmonisch. Kleine Verschiebungen um ein paar Hertz ergeben gegen das Direktsignal ein langsames, phasendes Schimmern; größere machen aus einer Note eine Glocke oder ein Kratzen. Es arbeitet als Einseitenband-Shifter, sodass das Ergebnis sauber bleibt, statt nach Ringmodulation zu klingen, und derselbe Oszillator treibt beide Kanäle, sodass das Stereobild hält. Setz es hinter ein Rhizome oder einen Sampler, oder hinter ein Fern, damit die Wiederholungen von der Note weggleiten.

## Parameter

**ShiftFrequency** Wie viele Hertz addiert werden, positiv oder negativ. Negative Verschiebungen schieben die Teiltöne zusammen, und tiefe Obertöne können durch null hindurch nach unten und wieder nach oben gedrückt werden.

**WetDryMix** Wie viel vom verschobenen Signal gegenüber dem Original zu hören ist. Kleine Verschiebungen erzeugen gegen das Direktsignal langsame Schwebungen; nur Effektsignal lässt die ursprüngliche Tonhöhe ganz hinter sich.

## Rezept

**Langsames Schimmern** Schick einen gehaltenen Pad-Klang hinein. ShiftFrequency 3, WetDryMix 0.5, und die verschobene Kopie schwebt gegen das Direktsignal des Pads etwa dreimal pro Sekunde. Leg ein Kabel von einem LFO mit kleiner Amplitude auf ShiftFrequency, damit die Schwebung driftet.

## Verwandte Organisms

Prism, SpectralMorph, Chorus
