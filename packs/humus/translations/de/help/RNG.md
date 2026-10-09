# RNG

Eine Quelle für Zufallszahlen aus der eigenen Entropie des Rechners, gesendet als Steuerwert.

Jede Ziehung stammt aus der Entropie des Systems statt aus einer Formel, die Folge lässt sich also weder wiederholen noch vorhersagen. Der Ausgang führt die letzte Ziehung; sie gelangt über ein Steuerkabel auf jede beliebige Buchse im Patch oder in ein Number, wo sie abgelesen und weitergereicht wird. Auf einem Regler zählt eine Ziehung in einer bestimmten Breite als Anteil dieser Breite, 200 von 255 steht also fast ganz oben; auf einem Number erscheint sie als die Zahl selbst. Die Anzeige unten zeigt die Ziehung, und die Lampe neben Rate blinkt bei jeder auf. Gezogen wird nach einer eigenen Clock, auf dem Schlag des Transports oder auf Abruf über Trigger.

## Parameter

**Rate** Ziehungen pro Sekunde, solange Sync aus ist, bis 50. Null hält die Clock an, sodass nur Trigger zieht.

**Sync** Nimmt die Clock für die Ziehungen vom Transport statt von Rate, sodass sie mit der Musik kommen und dem Tempo folgen. Während der Transport spielt, fällt eine Ziehung auf Schlag 1 und dann auf jede SyncBeats-Grenze, egal wohin du in der Timeline springst.

**SyncBeats** Musikalische Zeit zwischen den Ziehungen, solange Sync an ist: ein Viertel eines Schlags für etwas Unruhiges, vier für einen Wechsel pro Takt.

**Format** Welche Art Zahl eine Ziehung ist. 0 to 1 ist der einfache Wert zwischen null und eins, Coin ist dieser Wert auf null oder eins gerundet, und 8-bit, 8-bit signed, 16-bit und 16-bit signed ziehen eine ganze Zahl über diese Breite.

**Trigger** Zieht eine Zahl bei einer steigenden Flanke, sodass ein Button, eine Note oder ein Sensor nach einer Zahl fragen kann, statt auf die Clock zu warten.

## Rezept

**Neuer Cutoff in jedem Takt** Sync an, SyncBeats 4, Format 0 to 1. Zieh den Ausgang auf die Frequency-Buchse eines Filters hinter einem Drums-Organism und stell den Bereich der Verbindung auf 300 bis 3000; der Cutoff springt zu Beginn jedes Takts an eine neue Stelle.

## Verwandte Organisms

Number, Slider, LFO, Steps
