# LFO

Ein Niederfrequenz-Oszillator, der eine Welle als Steuersignal für die Regler anderer Organisms ausgibt.

Er gibt Offset plus Amplitude mal die Welle bei Rate aus, addiert zu dem, was an seinem Eingang ankommt, sodass ein LFO um den Wert eines Numbers herum schwanken kann. Mit Sync an dauert ein Zyklus SyncBeats Schläge des Transports, statt mit Rate zu laufen. Die Oberfläche zeigt einen Zyklus der Welle, die er gerade ausgibt, mit einer Abspielposition, die darauf mitläuft. Klick mit rechts auf einen beliebigen Parameter, und das Menü Steuern mit bietet den LFO zweimal an: wave ist die Welle selbst und phase die steigende Rampe des Zyklus. Zieh seinen Ausgang auf die Buchse eines Gains für ein Tremolo oder auf die Rate eines anderen LFOs für eine driftende Bewegung. Er führt kein Audio.

## Parameter

**Rate** Zyklen pro Sekunde, solange Sync aus ist, von einem alle hundert Sekunden bis 1000 Hz. Das obere Ende des Bereichs ist Audiorate.

**Waveform** SIN, TRI, SQR, SAW, DSAW (ein fallender Sägezahn) oder S&H, das einmal pro Zyklus auf einen neuen Zufallswert springt.

**Amplitude** Wie weit die Welle zu beiden Seiten von Offset ausschlägt.

**Offset** Die Mitte, um die sich die Welle bewegt. Heb sie an, damit ein bipolares Schwanken positiv bleibt.

**Sync** Koppelt einen Zyklus an den Transport statt an Rate. Solange der Transport läuft, beginnt der Zyklus auf Schlag 1 und folgt jedem Sprung, jedem Loop und jedem Scrubben; ist er gestoppt, läuft die Welle im Tempo weiter.

**SyncBeats** Wie viele Schläge ein Zyklus dauert, solange Sync an ist. Ein Schlag ist eine Viertelnote, also ist 0.5 eine Achtel und 4 ein Takt mit vier Schlägen.

## Rezept

**Synchrones Tremolo** Sync an, SyncBeats 0.5, Waveform SIN, Amplitude 0.4, Offset 0.6. Zieh den Ausgang auf die Gain-Buchse eines Gains, der eine Fläche führt, und der Pegel pulsiert in Achteln; schalte auf SQR für ein hartes Gate.

## Verwandte Organisms

Number, Follower, Sig
