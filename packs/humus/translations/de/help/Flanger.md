# Flanger

Ein wanderndes kurzes Delay, das das Signal mit einer Kopie seiner selbst zum Kammfilter macht.

Eine Kopie des Eingangs wird um einen Bruchteil einer Millisekunde verzögert und wieder dazugemischt, was manche Frequenzen auslöscht und andere verstärkt, in einer Reihe gleichmäßig verteilter Kerben. Ein Sinus-Sweep bewegt diese Verzögerung zwischen den beiden Enden von FrequencyRange, sodass der ganze Kamm im Spektrum auf und ab gleitet, und Feedback führt die verzögerte Kopie zum Eingang zurück, sodass sich jede Kerbe zu einer Resonanz schärft. Setz ihn bei Drums, gehaltenen Akkorden oder Rauschen ein, wo der wandernde Kamm oft die einzige Tonhöhe ist, die der Klang hat. Chorus verzögert länger und klingt nach Verstimmung; Phaser erzeugt weniger, ungleichmäßig verteilte Kerben, indem er stattdessen die Phase verschiebt.

## Parameter

**FrequencyRange** Die beiden Enden des Sweeps, als Frequenzen. Eine tiefe Frequenz ist ein langes Delay und ein dichter Kamm, eine hohe ein kurzes Delay und ein lichter, pfeifender Kamm; schieb die Griffe nah zusammen für eine feste metallische Färbung statt eines Sweeps.

**Rate** Wie schnell der Sweep zwischen den beiden Enden wandert, in Zyklen pro Sekunde.

**Feedback** Wie viel der verzögerten Kopie zum Eingang zurückkehrt. Wenig ist ein sanftes Rauschen, viel ist die kreischende, resonante Variante.

**WetDryMix** Wie viel der verzögerten Kopie gegenüber dem Original zu hören ist. Der Kamm entsteht nur, wo beide zusammen zu hören sind, deshalb liegt in der Mitte die tiefste Auslöschung, und voll Effektsignal entfernt den Effekt.

## Rezept

**Jet-Fill** Setz ihn hinter ein Drums-Organism. FrequencyRange 100 bis 3000, Rate 0.1, Feedback 0.7, WetDryMix 0.5. Verbinde einen LFO mit Rate, wenn der Sweep selbst über eine Phrase schneller und langsamer werden soll.

## Verwandte Organisms

Chorus, Phaser, Fern
