# SpectralFreeze

Ein Freeze und Smear per Phase-Vocoder, der ein Spektrum festhält und es dabei in Bewegung hält.

Smear dehnt, wie lange der Pegel jedes Bands über die Analyse-Frames hinweg anhält, und Freeze hält das aktuelle Spektrum fest und ignoriert den Eingang, bis es wieder freigegeben wird. Gehaltene Bänder behalten den an der Quelle gemessenen Phasenfortschritt, sodass ein eingefrorener Akkord weiter schwebt und driftet. Diffusion verwischt das gehaltene Spektrum über benachbarte Bänder und fügt ein langsames Wandern der Phase hinzu; Shimmer mischt eine Kopie eine Oktave höher wieder hinein. Smear 0 mit Freeze aus lässt das Signal exakt durch, und der Direktweg wird passend verzögert. Verkable ein Substrate, einen Sampler oder eine Gesangsstimme hinein und schick den Ausgang weiter an ein Verbatim oder ein Fern.

## Parameter

**Freeze** Greift das Spektrum, das gerade spielt, und hält es fest, wobei der Eingang ignoriert wird, bis du es ausschaltest.

**Smear** Wie lange der Pegel jedes Bands anhält, von etwa einer Millisekunde bis zu zehn Sekunden. Bei 0 geht der Eingang unverändert durch; höhere Werte lassen eine immer längere spektrale Fahne wachsen. Das ist der Regler zum Automatisieren.

**Diffusion** Verwischt die gehaltenen Pegel über die Bänder und fügt einen langsamen, zufälligen Gang der Phase hinzu, sodass ein Freeze aufblüht, statt stillzustehen. Gib es erst nach dem Einfrieren dazu, sonst verwischt es die Quelle, bevor du sie einfängst.

**Shimmer** Mischt eine Kopie des gehaltenen Spektrums eine Oktave höher wieder hinein.

**Mix** Wie viel vom eingefrorenen Signal gegenüber dem Direktsignal zu hören ist.

## Rezept

**Gehaltener Drone** Smear 0.7, Diffusion 0.3, Shimmer 0.2, Mix 1. Verkable einen Sampler oder ein gehaltenes Pad hinein, spiel einen Akkord und schalte Freeze genau in dem Moment ein, den du behalten willst; weise Freeze einem Button oder einer MIDI-Note zu, um ihn im Timing zu greifen. Schalte Freeze aus, um den Eingang wieder durchzulassen.

## Verwandte Organisms

SpectralFilter, SpectralMorph, SoundSpace, Substrate
