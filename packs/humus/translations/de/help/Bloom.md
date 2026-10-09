# Bloom

Der Shimmer-Hall: ein modulierter Hallraum mit einem Pitch-Shifter in seiner Feedback-Schleife.

Jeder Durchlauf durch den Hallraum wird erneut transponiert, sodass eine Hallfahne um eine Oktave steigt, dann um zwei, bis die Dämpfung sie schluckt. Das Verhältnis zwischen dem normalen und dem verschobenen Feedback ist Shimmer: Bei null ist das ein gewöhnlicher Hall, bei voll ist die Fahne nur Aufstieg und kein Körper. Feedback bestimmt, wie lange das Ganze klingt. Dreh Shift ins Negative, und die Fahne sinkt stattdessen. Setz es per Kabel hinter ein Rhizome, eine Wave oder einen Sampler, und setz ein Fern davor für eine Fahne, die auch echot.

## Parameter

**Mix** Direktsignal gegen Effektsignal.

**Shift** Wie weit der Shifter jeden Durchlauf transponiert, in Halbtönen. 12 ist die klassische Oktave, 7 schichtet Quinten, -12 sinkt.

**Shimmer** Wie viel des verschobenen Signals um den Hallraum zurückgeführt wird, im Verhältnis zur unverschobenen Fahne.

**Feedback** Wie viel des Hallraums an seinen eigenen Eingang zurückgeht. Das ist die Länge der Fahne.

**Size** Die Maße des Raums, von klein bis riesig.

**Diffusion** Wie schnell die frühen Echos verschwimmen. Niedrig hält einzelne Echos innerhalb der Fahne hörbar.

**LowCut** Entfernt Tiefen aus dem Eingang des Hallraums, damit das Anschwellen nicht in den Bass gerät.

**HighCut** Dunkelt die Fahne bei jedem Durchlauf ab. Niedriger ergibt eine kürzere, wärmere Fahne.

**ModRate** Die Geschwindigkeit der Bewegung im Hallraum, in Hz.

**ModDepth** Wie weit diese Bewegung geht. Ein wenig hält die Fahne lebendig, viel macht sie zu Wasser.

**Mode** Der Raumtyp. Bloom ist klein und nah, Hall ist natürlich, Cathedral ist riesig, Cloud liegt dazwischen, mit zusätzlicher Diffusion.

**Voice** Die Anordnung des Shifters. Single folgt Shift, Dual fügt eine zweite Stimme hinzu, die sich in die Gegenrichtung bewegt, Stacked fügt eine zweite Stimme beim doppelten Shift hinzu, und Off ist ein normaler Hall, egal was Shift sagt.

**Color** Die Neigung der Fahne. Bright hält die verschobenen Kopien offen, Dark dämpft sie, Neutral liegt dazwischen.

## Rezept

**Pad-Halo** Mode Cathedral, Voice Single, Shift 12, Shimmer 0.6, Feedback 0.6, HighCut 6000, Color Dark, Mix 0.4. Schick ein langsames Rhizome-Pad per Kabel hinein und halte einen Akkord ein oder zwei Takte lang, während sich die Oktaven aufbauen.

## Verwandte Organisms

Fern, Prism, SpectralFreeze
