# Phaser

Ein sechsstufiger Phaser, dessen Kerben zwischen zwei Frequenzen wandern.

Eine Kette von Allpassfiltern lässt den Pegel in Ruhe und verschiebt nur die Phase; mit dem Direktsignal zurückgemischt, löschen sich die Stellen, an denen die beiden nicht übereinstimmen, zu Kerben aus, und ein Sinus-Sweep bewegt diese Kerben zwischen den Enden von FrequencyRange. Weil nichts zeitlich verzögert wird, bleiben Transienten genau dort, wo sie waren, sodass er auf einem ganzen Submix sitzen kann, ohne ihn zu verschmieren. Wo die Kerben eines Flangers gleichmäßig verteilt und metallisch klingen, sind die eines Phasers wenige und ungleichmäßig, und das ergibt den subtileren, stimmhafteren Sweep. Setz ihn hinter ein E-Piano, ein Pad oder einen Mixer-Bus.

## Parameter

**FrequencyRange** Das untere und das obere Ende des Sweeps. Ein schmaler Bereich ist ein langsames tonales Wabern; ein weiter ist der volle Sweep von ganz unten bis ganz oben.

**Rate** Wie schnell die Kerben zwischen den Enden wandern, in Hz.

**Feedback** Führt den Ausgang zum Eingang zurück und schärft die Kerben zu resonanten Spitzen.

**Depth** Wie viel des phasenverschobenen Signals mit dem Direktsignal gemischt wird. Null ist reines Direktsignal; die tiefsten Kerben liegen nahe der Mitte, wo beide gleich stark sind.

## Rezept

**Langsamer Pad-Sweep** FrequencyRange 200 bis 2000, Rate 0.15, Feedback 0.5, Depth 0.5. Schließ ein gehaltenes Rhizome-Pad an, und der Sweep braucht etwa sieben Sekunden für einen Durchgang; schieb Feedback Richtung 0.8 für eine schärfere Spitze.

## Verwandte Organisms

Flanger, Chorus, Filter
