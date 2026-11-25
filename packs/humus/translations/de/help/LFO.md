# LFO

Ein Niederfrequenz-Oszillator, der eine Welle als Steuersignal für die Regler anderer Organisms ausgibt.

Er gibt Offset plus Amplitude mal die Welle bei Rate aus, addiert zu dem, was an seinem Eingang ankommt, sodass ein LFO um den Wert eines Numbers herum schwanken kann. Mit Sync an dauert ein Zyklus SyncBeats Schläge des Transports, statt mit Rate zu laufen. FN spielt eine eigene Form: Zieh einen Math auf die Buchse Function, und der LFO spielt die Formel des Math als seine Welle, oder zieh einen Wave hinein, und er spielt die Tabelle des Wave. Die Oberfläche zeigt einen Zyklus der Welle, die er gerade ausgibt, mit einer Abspielposition, die darauf mitläuft, oder auf FN seine jüngste Bewegung. Klick mit rechts auf einen beliebigen Parameter, und das Menü Steuern mit bietet den LFO zweimal an: wave ist die Welle selbst und phase die steigende Rampe des Zyklus. Zieh seinen Ausgang auf die Buchse eines Gains für ein Tremolo oder auf die Rate eines anderen LFOs für eine driftende Bewegung. Er führt kein Audio.

## Parameter

**Rate** Zyklen pro Sekunde, solange Sync aus ist, von einem alle hundert Sekunden bis 1000 Hz. Das obere Ende des Bereichs ist Audiorate. Auf FN ist sie auch rate in der Formel, sodass eine Bewegung, die um rate*dt weiterschreitet, in ihrem Tempo läuft.

**Waveform** SIN, TRI, SQR, SAW, DSAW (ein fallender Sägezahn), S&H, das einmal pro Zyklus auf einen neuen Zufallswert springt, oder FN, die über Function übergebene Formel oder Tabelle.

**Amplitude** Wie weit die Welle zu beiden Seiten von Offset ausschlägt.

**Offset** Die Mitte, um die sich die Welle bewegt. Heb sie an, damit ein bipolares Schwanken positiv bleibt.

**Sync** Koppelt einen Zyklus an den Transport statt an Rate. Solange der Transport läuft, beginnt der Zyklus auf Schlag 1 und folgt jedem Sprung, jedem Loop und jedem Scrubben; ist er gestoppt, läuft die Welle im Tempo weiter.

**SyncBeats** Wie viele Schläge ein Zyklus dauert, solange Sync an ist. Ein Schlag ist eine Viertelnote, also ist 0.5 eine Achtel und 4 ein Takt mit vier Schlägen.

**Function** Zieh hier einen Math hinein, um seine Formel an den LFO zu übergeben. Das Kabel trägt die Formel und die Regler X, Y, Z und W des Math statt eines Werts: Der LFO schaltet auf FN, folgt jeder Änderung, solange das Kabel steckt, und behält, was er hat, wenn es gezogen wird. In der Formel ist phase der Zyklus des LFOs und rate seine Rate, sodass die Nature-Presets eines Math im Tempo des LFOs laufen. Ein hineingezogener Wave übergibt stattdessen seine Tabelle: Der LFO spielt die Frames an der Position des Wave und folgt diesem Regler.

## Rezept

**Synchrones Tremolo** Sync an, SyncBeats 0.5, Waveform SIN, Amplitude 0.4, Offset 0.6. Zieh den Ausgang auf die Gain-Buchse eines Gains, der eine Fläche führt, und der Pegel pulsiert in Achteln; schalte auf SQR für ein hartes Gate.

**Springender Ball** Lade das Nature-Preset Gravity in einen Math, zieh den Math auf die Buchse Function des LFOs und den Ausgang des LFOs auf den Cutoff eines Filters. Der Klang folgt einem Ball, der immer früher und niedriger springt, bis er liegen bleibt, und dann wieder hochgeworfen wird; Rate beschleunigt den Ball und das X des Math macht ihn lebhafter.

**Wandernde Welle** Schreib `walk := walk*(1 - rate*dt) + sqrt(6*rate*dt)*noise(); sin(tau*ph(rate*(1 + 0.15*x*walk)))` in einen Math und zieh ihn auf Function: ein Sinus bei der Rate des LFOs, dessen Tempo driftet wie eine Hand am Regler, die sich nie ganz wiederholt.

## Verwandte Organisms

Math, Number, Follower, Sig
