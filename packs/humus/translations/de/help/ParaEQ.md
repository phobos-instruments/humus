# ParaEQ

Ein Vierband-Equalizer mit einem Shelf an jedem Ende und zwei durchstimmbaren Bändern in der Mitte.

Die beiden Shelves heben oder senken alles jenseits ihrer Eckfrequenz und kippen so den ganzen Klang; die beiden Mittenbänder wirken auf einen Bereich um ihre Mittenfrequenz, greifen also gezielt hinein und ändern nur eine Stelle. Alle vier laufen auf jedem Kanal hintereinander. Setz ihn per Kabel hinter einen Sampler, einen FilePlayer oder einen Mixer-Bus, um den Matsch aus einem Bass zu nehmen, eine klingelnde Frequenz in einer Raumaufnahme abzusenken oder etwas Dumpfem Luft zu geben. Eine kleine Absenkung dort, wo sich zwei Parts in die Quere kommen, bringt meist mehr als eine Anhebung bei einem der beiden. Die Bandbreite wird in Hertz angegeben, 200 Hz breit sind also 200 Hz breit, egal wo das Band sitzt, auch wenn dieselbe Breite in der Tiefe einen größeren Teil der Oktave abdeckt.

## Parameter

**HSCutoffFrequency** Die Eckfrequenz des High-Shelfs. Alles darüber bewegt sich gemeinsam.

**HSGain** Absenkung oder Anhebung des High-Shelfs in dB.

**BP1CenterFrequency** Die Mitte des ersten Mittenbands.

**BP1Bandwidth** Wie weit das erste Band reicht, in Hz. Schmal für eine einzelne Resonanz, breit für eine Klangentscheidung.

**BP1Gain** Absenkung oder Anhebung des ersten Bands in dB. Negativ ergibt eine Notch.

**BP2CenterFrequency** Die Mitte des zweiten Mittenbands.

**BP2Bandwidth** Die Breite des zweiten Bands in Hz.

**BP2Gain** Absenkung oder Anhebung des zweiten Bands in dB.

**LSCutoffFrequency** Die Eckfrequenz des Low-Shelfs. Alles darunter bewegt sich gemeinsam.

**LSGain** Absenkung oder Anhebung des Low-Shelfs in dB.

## Rezept

**Eine Resonanz finden** Stell BP1Gain auf 12 und BP1Bandwidth auf 60 und fahr dann mit BP1CenterFrequency durch, bis der Klang schlechter wird. Das ist die Frequenz: Stell BP1Gain dort auf -6 und mach das Band breiter, bis die Absenkung nicht mehr wie ein Loch klingt.

## Verwandte Organisms

Filter, Prism, SpectralFilter
